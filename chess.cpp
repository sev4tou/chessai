#include<iostream>
#include<string.h>
#include<gtk/gtk.h>
#include<sstream>
#include<sys/stat.h>

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
	BLANK,
	PAWN,
	ROOK,
	KNIGHT,
	BISHOP,
	QUEEN,
	KING
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

struct Board {
	constexpr static unsigned char startDat[32]{18, 0, 0, 169, 19, 0, 0, 185, 20, 0, 0, 201, 21, 0, 0, 217, 22, 0, 0, 233, 20, 0, 0, 201, 19, 0, 0, 185, 18, 0, 0, 169};
	unsigned char dat[32];

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

	inline int getEnpassant(int side, int row) {
		return dat[side*8+row]==0b1000;
	}
	inline int getRookMoved(int side, int islong) {
		return dat[side*2+islong+16]==0b1000;
	}
	inline int getKingMoved(int side) {
		return dat[side*2+20]==0b1000;
	}
	inline void upEnpassant(int side, int row) {
		dat[side*8+row]|=0b1000;
	}
	inline void upRookMoved(int side, int islong) {
		dat[side*2+islong+16]|=0b1000;
	}
	inline void upKingMoved(int side) {
		dat[side*2+20]|=0b1000;
	}
 
	inline void downEnpassant(int side, int row) {
		dat[side*8+row]&=0b0111;
	}
	inline void downRookMoved(int side, int islong) {
		dat[side*2+islong+16]&=0b0111;
	}
	inline void downKingMoved(int side) {
		dat[side*2+20]&=0b0111;
	}
};

struct Window {
	GtkApplication *app;
	cairo_surface_t *surface;
	int pieceSize = 128;
	rgba_t oddColor{1.0, 0, 0.5, 1};
	rgba_t evenColor{1.0, 0.5, 0.75, 1};
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

	Window() {
		app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
		g_signal_connect(app, "activate", G_CALLBACK (activate), this);
		assets = cairo_image_surface_create_from_png("asset.png");
		flipAssets = cairo_image_surface_create_from_png("asset.png");
		flipAssets = cairo_image_surface_create_from_png("asset.png");
		cairo_surface_t *tmp = cairo_image_surface_create_from_png("asset.png");

		cairo_t *cr = cairo_create(tmp);
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_set_operator (cr, CAIRO_OPERATOR_DIFFERENCE);
		cairo_paint(cr);
		cairo_destroy(cr);

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
		redraw();
		gtk_widget_queue_draw(widget);
	}
};

int main(int argc, char **argv) {
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

	Window w = EditWindow();
	w.board = b;
	return w.run(argc, argv);
}

void Window::activate(GtkApplication *app, gpointer data) {
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *drawingArea;
	int widSize = 8*((Window*)data)->pieceSize + 2;

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
