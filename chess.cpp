#include<iostream>
#include<array>
#include<string.h>
#include<gtk/gtk.h>
#include<sstream>
#include<sys/stat.h>
#ifndef G_APPLICATION_DEFAULT_FLAGS 
#define G_APPLICATION_DEFAULT_FLAGS G_APPLICATION_FLAGS_NONE
#endif

namespace common {
	bool fileExisted(const char *file) {
		struct stat buf;
		return stat(file, &buf) == 0;
	}

	FILE *openWriteSafe(const char *file) {
		std::stringstream ss;
		ss<<file;
		int i=0;
		while(fileExisted(ss.str().c_str())) {
			ss=std::stringstream();
			ss<<file<<'('<<++i<<')';
		}
		FILE *ret = fopen(ss.str().c_str(), "w");
		if(!ret) {
			perror("o vcl :vvv");
			exit(-1);
		}
		return ret;
	}
};

enum PIECE {
	BLANK = 0,
	PAWN = 1,
	ROOK = 2,
	KNIGHT = 3, 
	BISHOP = 4,
	QUEEN = 5,
	KING = 6
};

enum SIDE {
	WHITE = 0,
	BLACK = 8
};

struct rgba_t {
	float a;
	float b;
	float g;
	float r;
};

static unsigned char startDat[32]{18, 0, 0, 169, 19, 0, 0, 185, 20, 0, 0, 201, 21, 0, 0, 217, 22, 0, 0, 233, 20, 0, 0, 201, 19, 0, 0, 185, 18, 0, 0, 169};

struct Board {
	unsigned char dat[32];
	unsigned char flg[4]{};

	static void inline setMove(unsigned char *buf, int id, int x, int y) {
		buf[id] = (x<<3) | y;
	}

	inline void move(int x1, int y1, int x2, int y2) {
		downEnpassant(!getWhiteTurn());
		switch(getPiece(x1,y1)&0b0111) {
			case PIECE::PAWN:
				if(y1-y2==2||y1-y2==-2) upEnpassant(!getWhiteTurn(), x1);
				if(getPiece(x2,y2)==0&&x2!=x1) setPiece(x2,y1,0);
				break;
			case PIECE::KING:
				upKingMoved(!getWhiteTurn());
				break;
			case PIECE::ROOK:
				if(y1 == ((getPiece(x1, y1)&0b1000)?0:7))
					if(x1==0) upRookMoved(!getWhiteTurn(), 1);
					else if(x1==7) upRookMoved(!getWhiteTurn(), 0);
				break;
		}
		setPiece(x2,y2,getPiece(x1,y1)&0b1111);
		setPiece(x1,y1,0);
		flg[2]^=(1<<6);
	}
	void listMove(const int x, const int y, unsigned char *buf, int preview) {
		int p = getPiece(x, y);
		int s = 1 - ((p&0b1000)>>2);
 		static const int drx[]={1,-1,0,0};
		static const int dry[]={0,0,-1,1};
		 
		memset(buf, -1, 32);
		switch (p & 0b111) {
			case PIECE::BLANK:
				break;
			case PIECE::PAWN:
				if((getPiece(x, y+s)&0b0111) == PIECE::BLANK) {
					setMove(buf, 0, x, y+s);
					if(y == (s==1? 1 : 6) && (getPiece(x, y+2*s)&0b0111) == PIECE::BLANK)
						setMove(buf, 1, x, y+2*s);
				}
				if(x<7 && ((getPiece(x+1, y+s)&0b111) != PIECE::BLANK) && ((getPiece(x+1, y+s)&0b1000) ^ (p&0b1000)))
					setMove(buf, 2, x+1, y+s);
				if(x>0 && ((getPiece(x-1, y+s)&0b111) != PIECE::BLANK) && ((getPiece(x-1, y+s)&0b1000) ^ (p&0b1000)))
					setMove(buf, 3, x-1, y+s);
            if(x<7 && y == (s==1? 4 : 3) && ((getPiece(x+1, y)) == (PIECE::PAWN | ((p&0b1000)^0b1000))) && getEnpassant(!((p&0b1000)>>3), x+1))   
					setMove(buf, 4, x+1, y+s);
            if(x>0 && y == (s==1? 4 : 3) && ((getPiece(x-1, y)) == (PIECE::PAWN | ((p&0b1000)^0b1000))) && getEnpassant(!((p&0b1000)>>3), x-1)) 
					setMove(buf, 5, x-1, y+s);
				break;
			case PIECE::KNIGHT:
				static const int dx[]={2,2,-2,-2,1,1,-1,-1};
				static const int dy[]={1,-1,1,-1,2,-2,2,-2};
				for(int i=0;i<8;++i) {
					int u = x+dx[i], v = y+dy[i];
					if(u>=0 && u<8 && v>=0 && v<8 && !((getPiece(u,v)&0b111) && (getPiece(u,v)&0b1000)==(p&0b1000)))
							setMove(buf, i, u, v);
				}
				break;
		   case PIECE::QUEEN:
				for(int i=0,j=16;i<4;++i) {
					int u=x, v=y;
					while(true) {
						u+=drx[i];
						v+=dry[i];
						if(u>=0 && u<8 && v>=0 && v<8 && !((getPiece(u,v)&0b111) && (getPiece(u,v)&0b1000)==(p&0b1000))) {
							setMove(buf, j++, u, v);
							if(getPiece(u,v)&0b111) break;
						}
						else break;
					}
				} 
				[[fallthrough]]
			case PIECE::BISHOP:
				static const int dbx[]={1,1,-1,-1};
				static const int dby[]={1,-1,1,-1};
				for(int i=0,j=0;i<4;++i) {
					int u=x, v=y;
					while(true) {
						u+=dbx[i];
						v+=dby[i];
						if(u>=0 && u<8 && v>=0 && v<8 && !((getPiece(u,v)&0b111) && (getPiece(u,v)&0b1000)==(p&0b1000))) {
							setMove(buf, j++, u, v);
							if(getPiece(u,v)&0b111) break;
						}
						else break;
					}
				}
				break;
			case PIECE::ROOK:
				for(int i=0,j=0;i<4;++i) {
					int u=x, v=y;
					while(true) {
						u+=drx[i];
						v+=dry[i];
						if(u>=0 && u<8 && v>=0 && v<8 && !((getPiece(u,v)&0b111) && (getPiece(u,v)&0b1000)==(p&0b1000))) {
							setMove(buf, j++, u, v);
							if(getPiece(u,v)&0b111) break;
						}
						else break;
					}
				}                            
				break;
			case PIECE::KING: {
				std::array<bool, 64> mark{};
				if(!preview)
					for(int i=0;i<8;++i)
						for(int j=0; j<8;++j)
							if(((getPiece(i,j)&0b1000)^(p&0b1000)) && (getPiece(i,j)&0b111)) {
								unsigned char a[32];
								listMove(i, j, a, 1);
								for(int i=0;i<32;++i)
									if(a[i]!=0b11111111)
										mark[a[i]]=1;
							}
			 
				for(int i=0;i<4;++i) {
					int u = x+drx[i], v = y+dry[i];
					if(u>=0 && u<8 && v>=0 && v<8 && !((getPiece(u,v)&0b111) && (getPiece(u,v)&0b1000)==(p&0b1000)) && !mark[(u<<3)|v])
						setMove(buf, i, u, v);
				}
				for(int i=0;i<4;++i) {
					int u = x+dbx[i], v = y+dby[i];
					if(u>=0 && u<8 && v>=0 && v<8 && !((getPiece(u,v)&0b111) && (getPiece(u,v)&0b1000)==(p&0b1000)) && !mark[(u<<3)|v])  
						setMove(buf, i+4, u, v);
				}
				if(!getKingMoved(p&0b1000>>3) && !mark[(x<<3)|y] && !preview) {
					if(!getRookMoved((p&0b1000)>>3, 1)) {
						if((getPiece(1,y)&0b0111) == PIECE::BLANK) {
							int i;
							for(i=2;i<=3;i++)
								if((getPiece(i,y)&0b0111) != PIECE::BLANK || mark[(i<<3)|y]) break;
							if(i==4) setMove(buf, 9, 2, y);
						}
					}
					if(!getRookMoved((p&0b1000)>>3, 0)) {
						int i;
						for(i=5;i<=6;i++)
							if((getPiece(i,y)&0b0111) != PIECE::BLANK || mark[(i<<3)|y]) break;
						if(i==7) setMove(buf, 10, 6, y);
					}
				}
				break;
			}
		}


		if(!preview)
			for(int i=0;i<32;++i) {
				if(buf[i]==0b11111111)continue;
				Board tmp=*this;
				tmp.move(x,y,buf[i]/8, buf[i]%8);
				if(tmp.isCheck(p&0b1000))
					buf[i]=0b11111111;
			}
	}

	bool isCheck(int side) {
		for(int i=0;i<8;++i)
			for(int j=0;j<8;++j)
				if(((getPiece(i,j)&0b1000)^side) && (getPiece(i,j)&0b111)) {
					unsigned char a[32];
					listMove(i, j, a, 1);
					for(int i=0;i<32;++i)
						if(a[i]!=0b11111111)
							if(getPiece(a[i]/8,a[i]%8)==((side)|(PIECE::KING)))
								return true;
				}
		return false;
	}

	void exportBoard(const char *str) {
		FILE *f = fopen(str, "w");
      if(!f) {
			perror("fopen()");
			return;
		}
		if(fwrite(dat, 32, 1, f)!=1) 
			perror("fwrite()");
		fclose(f);
	}
	void importBoard(const char *str) {
		FILE *f = fopen(str, "r");
		if(!f) {
			perror("fopen()");
			return;
		}
		if(fread(dat, 32, 1, f)!=1)
			perror("fread()");
		fclose(f);
	}
	inline void setPiece(int x, int y, int v) {
		const int id = (x<<3)|y;
		const int mask = (0b1111) << ((y&1)<<2);
		dat[id/2]&=0b11111111^mask;
		dat[id/2]|=(v)<<((y&1)<<2);
	}

	inline int getPiece(int x, int y) {
		const int id = (x<<3)|y;
		const int mask = (0b1111) << ((y&1)<<2);
		return(dat[id/2]&mask)>>((y&1)<<2);
	}

	inline bool getEnpassant(int side, int row) {
		return flg[side]&(1<<row);
	}
	inline bool getRookMoved(int side, int islong) {
		return flg[2]&(1<<((side<<1)+islong)); 
	}
	inline bool getKingMoved(int side) {
		return flg[2]&(1<<(side+4));
	}
	inline bool getWhiteTurn() {
		return flg[2]&(1<<6);
	}

	inline void upEnpassant(int side, int row) {
		flg[side]|=(1<<row);
	}
	inline void upRookMoved(int side, int islong) {
		flg[2]|=(1<<((side<<1)+islong)); 
	}
	inline void upKingMoved(int side) {
		flg[2]|=(1<<(side+4));
	}
	inline void upWhiteTurn() {
		flg[2]|=(1<<6);
	}

	inline void downEnpassant(int side) {
		flg[side]=0;
	}
	inline void downRookMoved(int side, int islong) {
		flg[2]&=(1<<((side<<1)+islong))^0b11111111; 
	}
	inline void downKingMoved(int side) {
		flg[2]&=(1<<(side+4))^0b11111111;
	}
	inline void downWhiteTurn() {
		flg[2]&=(1<<6)^0b11111111;
	}
};

struct Window {
	GtkApplication *app;
	cairo_surface_t *surface=0;
	int pieceSize = 64;
	rgba_t oddColor{1.0, 0, 0.5, 1};
	rgba_t evenColor{1.0, 0.5, 0.75, 1};
	rgba_t custom[8][8]{};
	cairo_surface_t *assets;
	cairo_surface_t *flipAssets;
	Board board;

	static void activate(GtkApplication *app, gpointer data);
	static void closeWindow(GtkWidget *widget, gpointer data);
	static gboolean motionNotifyEvent(GtkWidget *widget, GdkEventMotion *event, gpointer data);
	static gboolean buttonPressEvent(GtkWidget *widget, GdkEventButton *event, gpointer data);
	static gboolean draw(GtkWidget *widget, cairo_t *cr, gpointer data);
	static gboolean configureEvent(GtkWidget *widget, GdkEventConfigure *event, gpointer data);

	void virtual motionNotify(GtkWidget *widget, GdkEventMotion *event);
	void virtual buttonPress(GtkWidget *widget, GdkEventButton *event);
	void virtual redraw();
	int virtual getWidth() { return 8*pieceSize; }
	int virtual getHeight() { return 8*pieceSize; }

	Window() {
		app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
		g_signal_connect(app, "activate", G_CALLBACK (activate), this);

		cairo_surface_t *tmp = cairo_image_surface_create_from_png("asset.png");
		assets = cairo_surface_create_similar(tmp, CAIRO_CONTENT_COLOR_ALPHA, pieceSize*8, pieceSize);
		cairo_t *cr = cairo_create(assets);
		cairo_scale(cr, (float)pieceSize/128, (float)pieceSize/128);
		cairo_set_source_surface(cr, tmp, 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);
		cairo_surface_destroy(tmp);

		tmp = cairo_surface_create_similar(assets, CAIRO_CONTENT_COLOR_ALPHA, pieceSize*8, pieceSize);
		cr = cairo_create(tmp);
		cairo_set_source_surface(cr, assets, 0, 0);
		cairo_paint(cr);
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
		cairo_paint(cr);
		cairo_destroy(cr);

		flipAssets = cairo_surface_create_similar(tmp, CAIRO_CONTENT_COLOR_ALPHA, pieceSize*8, pieceSize);
		cr = cairo_create(flipAssets);
		cairo_set_source_surface(cr, tmp, 0, 0);
		cairo_mask_surface(cr, assets, 0, 0);
		cairo_fill(cr);
		cairo_destroy(cr);
		cairo_surface_destroy(tmp);

 }

	int run(int argc, char **argv) {
		int status;
		status = g_application_run(G_APPLICATION(app), argc, argv);
		g_object_unref(app);
		return status;
	}
};

struct EditWindow : public Window {
	int editId = 0;
	int virtual getWidth() override { return 11*pieceSize; }
	void virtual redraw() override {
		Window::redraw();
		cairo_t *cr = cairo_create(surface);
		int idd = 0;
		for(int i=9;i<11;++i)
			for(int j=0;j<7;++j) {
			const int piece = (i-9)*8+j;
			if((i+j)&1) cairo_set_source_rgb(cr, oddColor.r, oddColor.g, oddColor.b);
			else cairo_set_source_rgb(cr, evenColor.r, evenColor.g, evenColor.b);

			if(editId == piece) {
            cairo_set_source_rgb(cr, 0.4, 0.9, 0.4);  
			}

			cairo_rectangle(cr, i*pieceSize, j*pieceSize, pieceSize, pieceSize);
			cairo_fill(cr);
			int id=piece&0b111;
			if(id) {
				cairo_surface_t *sur = (piece&0b1000)?assets:flipAssets;
				cairo_set_source_surface(cr, sur, (i-id)*pieceSize, j*pieceSize);
				cairo_rectangle(cr, i*pieceSize, j*pieceSize, pieceSize, pieceSize);
				cairo_fill(cr);
			}
		}
		cairo_set_source_rgb(cr, 0.01, 0.05, 0.6);
		cairo_rectangle(cr, 8*pieceSize, 0, pieceSize, pieceSize);
		cairo_fill(cr);
 
		cairo_set_source_rgb(cr, 0.6, 0.15, 0.1);
		cairo_rectangle(cr, 8*pieceSize, 1*pieceSize, pieceSize, pieceSize);
		cairo_fill(cr);
		
		cairo_destroy(cr);
	}
	void virtual buttonPress(GtkWidget *widget, GdkEventButton *event) {
		int i = event->x/pieceSize;
		int j = event->y/pieceSize;
		if(event->button == 1) {
			if(i<8 && j<8 && i>=0 && j>=0) {
				board.setPiece(i, j, editId);
			}
			if(i>=9 && i<11 && j>=0 && j<7 && !(i==10 && j==0)) {
				editId = (i-9)*8+j;
			}
			if(i==8&&j==0) {
				char *filename;
				GtkWidget *dialog;
				GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
				gint res;

				dialog = gtk_file_chooser_dialog_new("Open File",
						NULL,
						action,
						"_Cancel",
						GTK_RESPONSE_CANCEL,
						"_Open",
						GTK_RESPONSE_ACCEPT,
						NULL);

				gtk_file_chooser_set_current_folder((GtkFileChooser*)dialog, get_current_dir_name());
				res = gtk_dialog_run (GTK_DIALOG(dialog));
				if (res == GTK_RESPONSE_ACCEPT)
				{
					GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
					filename=gtk_file_chooser_get_filename(chooser);
					board.importBoard(filename);
					g_free(filename);
				}

				gtk_widget_destroy(dialog);
				fflush(stdout);
			}
			if(i==8&&j==1) {
				char *filename;
				GtkWidget *dialog;
				GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
				gint res;

				dialog = gtk_file_chooser_dialog_new("Save File",
						NULL,
						action,
						"_Cancel",
						GTK_RESPONSE_CANCEL,
						"_Save",
						GTK_RESPONSE_ACCEPT,
						NULL);

				gtk_file_chooser_set_current_folder((GtkFileChooser*)dialog, get_current_dir_name());
				res = gtk_dialog_run (GTK_DIALOG(dialog));
				if (res == GTK_RESPONSE_ACCEPT)
				{
					GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
					filename=gtk_file_chooser_get_filename(chooser);
					board.exportBoard(filename);
					g_free(filename);
				}

				gtk_widget_destroy(dialog);
				fflush(stdout);   
			}
		}
		else if (event->button == 3) {
			memset(custom, 0, sizeof(custom));
			if(i<8 && j<8 && i>=0 && j>=0) {
				unsigned char buf[32];
				board.listMove(i, j, buf, 0);
				for(int i = 0; i<32; ++i) {
					int id1=buf[i];
					custom[id1/8][id1%8].r=1;
					custom[id1/8][id1%8].a=1;
				}
			}
		}
		redraw();
		gtk_widget_queue_draw(widget);
	}
};

//TODO
struct BoardValidator {

};

struct GameWindow : public Window {
   int selecting = 0;
	int oldi=-1, oldj=-1;
	unsigned char moveList[32];

	GameWindow() {
		memcpy(board.dat, startDat, sizeof(startDat));
	}

	void virtual buttonPress(GtkWidget *widget, GdkEventButton *event) {
   	int i = event->x/pieceSize;
		int j = event->y/pieceSize;

		if(event->button == 1) {
			if(selecting == 0) {
				if(i>=0 && i<8 && j>=0 && j<8) {
					if(board.getWhiteTurn() ^ ((board.getPiece(i, j)&0b1000)>>3)) {
						custom[i][j].a=0.5;
						custom[i][j].r=0.;
						custom[i][j].g=0.;
						custom[i][j].b=0.;
						oldi=i; oldj=j;
						selecting = 1;
						board.listMove(i, j, moveList, 0);
						for(int i = 0; i<32; ++i) {
							int id1=moveList[i];
							if(moveList[i]==0b11111111)continue;
							custom[id1/8][id1%8].g=1;
							custom[id1/8][id1%8].r=0.6;
							custom[id1/8][id1%8].b=0.6;
							custom[id1/8][id1%8].a=1;
						}
					}
				}
			}
			else {
				selecting = 0;
				memset(custom, 0, sizeof(custom));
				int it;
				for(it=0; it<32;++it) {
					if(moveList[it]==0b11111111)continue;
					if(moveList[it] == ((i<<3)|j)) break;
				}
				if(it<32) board.move(oldi, oldj, i, j);
			}
		}
		else if(event->button == 3) {
			selecting = 0;
			memset(custom, 0, sizeof(custom));
		}
		redraw();
		gtk_widget_queue_draw(widget);
	}
};


int main(int argc, char **argv) {
	using namespace std;
	srand(time(0));
	Board b{};
	//test 1
	for(int i=0; i<1000; ++i) {
		int x = rand() % 8;
		int y = rand() % 8;
		int v = rand() % 16;
		b.setPiece(x, y, v);
		if(v != b.getPiece(x,y)) 
			std::cout<<"ngu: "<<x<<' '<<y<<std::endl;
	}

	//test 2
	Window w = GameWindow();
	return w.run(argc, argv);
}

void Window::activate(GtkApplication *app, gpointer data) {
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *drawingArea;
	int widSize = ((Window*)data)->getWidth()+ 2;
	int heiSize = ((Window*)data)->getHeight()+ 2;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Stupid Chess");
	g_signal_connect(window, "destroy", G_CALLBACK(closeWindow), data);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(window), frame);

	drawingArea = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawingArea, widSize, widSize);
	gtk_container_add(GTK_CONTAINER(frame), drawingArea);
	g_signal_connect(drawingArea, "draw", G_CALLBACK(draw), data); 
	g_signal_connect(drawingArea, "configure-event", G_CALLBACK(configureEvent), data);
	g_signal_connect(drawingArea, "motion-notify-event", G_CALLBACK(motionNotifyEvent), data);
	g_signal_connect(drawingArea, "button-press-event", G_CALLBACK(buttonPressEvent), data);

	/* Ask to receive events the drawing area doesn't normally
	 * subscribe to. In particular, we need to ask for the
	 * button press and motion notify events that want to handle.
	 */
	gtk_widget_set_events(drawingArea, gtk_widget_get_events (drawingArea) | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
	gtk_widget_show_all(window);
}

void Window::closeWindow(GtkWidget *widget, gpointer data) {
	Window *w = (Window*)data;
	if(w->surface)
		cairo_surface_destroy(w->surface);
}

gboolean Window::draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	cairo_set_source_surface(cr, ((Window *)data)->surface, 0, 0);
	cairo_paint(cr);
	return FALSE;
}

gboolean Window::configureEvent(GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
	Window *w = (Window*)data;

	if(w->surface)
		cairo_surface_destroy(w->surface);
	w->surface = gdk_window_create_similar_surface(gtk_widget_get_window(widget),
			CAIRO_CONTENT_COLOR,
			gtk_widget_get_allocated_width(widget),
			gtk_widget_get_allocated_height(widget));
	w->redraw();
	return TRUE;
}

gboolean Window::motionNotifyEvent(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
	Window *w = (Window*)data;
	w->motionNotify(widget, event);
	return TRUE;
}

gboolean Window::buttonPressEvent(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	Window *w = (Window*)data;
	w->buttonPress(widget, event);
	return TRUE;
}

void Window::redraw() {
	cairo_t *cr = cairo_create(surface);
	for(int i=0;i<8;++i)
		for(int j=0;j<8;++j) {
			const int piece = board.getPiece(i, j);
			if((i+j)&1)
				cairo_set_source_rgb(cr, oddColor.r, oddColor.g, oddColor.b);
			else 
				cairo_set_source_rgb(cr, evenColor.r, evenColor.g, evenColor.b);
			if(custom[i][j].a!=0)
				cairo_set_source_rgba(cr, custom[i][j].r, custom[i][j].g, custom[i][j].b, custom[i][j].a);
			cairo_rectangle(cr, i*pieceSize, j*pieceSize, pieceSize, pieceSize);
			cairo_fill(cr);
			int id=piece&0b111;
			if(id) {
				cairo_surface_t *sur = (piece&0b1000)?assets:flipAssets;
				cairo_set_source_surface(cr, sur, (i-id)*pieceSize, j*pieceSize);
				cairo_rectangle(cr, i*pieceSize, j*pieceSize, pieceSize, pieceSize);
				cairo_fill(cr);
			}
		}
	cairo_destroy(cr);
}

void Window::buttonPress(GtkWidget *widget, GdkEventButton *event) {
}

void Window::motionNotify(GtkWidget *widget, GdkEventMotion *event) {
}
