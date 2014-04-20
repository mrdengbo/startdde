#include "background.h"
#include <gdk/gdkx.h>
#include <cairo/cairo-xlib.h>

gboolean background_info_draw_callback(GtkWidget* w, cairo_t* cr, BackgroundInfo* info)
{
    if (info->bg != NULL) {
        g_mutex_lock(&info->m);
	cairo_set_source_surface(cr, info->bg, 0, 0);
	cairo_paint_with_alpha(cr, info->alpha);
        g_mutex_unlock(&info->m);
    }

    GList* children = gtk_container_get_children(GTK_CONTAINER(w));
    for (GList* l = children; l != NULL; l = l->next) {
	GtkWidget* child = GTK_WIDGET(l->data);
	if (gtk_widget_get_visible(child)) {
	    gdk_cairo_set_source_window(cr, gtk_widget_get_window(child), 0, 0);
	    cairo_paint(cr);
	}
    }
    g_list_free(children);

    return TRUE;
}

void background_info_set_background_by_drawable(BackgroundInfo* info, guint32 drawable)
{
    gint x, y;
    Window root;
    guint border,depth, width=0, height=0;
    Display* dpy = gdk_x11_get_default_xdisplay();
    gdk_error_trap_push();
    XGetGeometry(dpy, drawable, &root, &x, &y, &width, &height, &border, &depth);
    if (gdk_error_trap_pop()) {
        g_warning("set_background_by_drawable invalid drawable %d \n", drawable);
        return;
    }

    g_mutex_lock(&info->m);
    if (info->bg != NULL) {
	cairo_surface_destroy(info->bg);
	info->bg = NULL;
    }
    info->bg = cairo_xlib_surface_create(
            dpy, drawable,
	    gdk_x11_visual_get_xvisual(gdk_visual_get_system()),
            width, height
            );
    g_mutex_unlock(&info->m);
    gdk_window_invalidate_rect(gtk_widget_get_window(info->container), NULL, FALSE);
}

void background_info_set_background_by_file(BackgroundInfo* info, const char* file)
{
    g_message("background_info_set_background_by_file:%s",file);
    GError* error = NULL;
    GdkPixbuf* pb = gdk_pixbuf_new_from_file(file, &error);
    if (error != NULL) {
	g_warning("set_background_by_file failed: %s\n", error->message);
	g_error_free(error);
	return;
    }
    g_mutex_lock(&info->m);
    if (info->bg != NULL) {
        cairo_surface_destroy(info->bg);
        info->bg = NULL;
    }
    info->bg = gdk_cairo_surface_create_from_pixbuf(pb, 1, gtk_widget_get_window(info->container));
    g_mutex_unlock(&info->m);
    g_object_unref(pb);
    gdk_window_invalidate_rect(gtk_widget_get_window(info->container), NULL, FALSE);
}

void background_info_change_alpha(BackgroundInfo* info, double alpha)
{
    info->alpha = alpha;
    gdk_window_invalidate_rect(gtk_widget_get_window(info->container), NULL, FALSE);
}

void background_info_clear(BackgroundInfo* info)
{
    if (info->bg != NULL) {
        cairo_surface_destroy(info->bg);
    }
    g_mutex_clear(&info->m);
    g_free(info);
}

BackgroundInfo* create_background_info(GtkWidget* container, GtkWidget* child)
{
    g_message("create_background_info");
    BackgroundInfo* info = g_new0(BackgroundInfo, 1);
    g_mutex_init(&info->m);
    info->alpha = 1;

    gtk_widget_realize (child);
    gdk_window_set_composited(gtk_widget_get_window(child), TRUE);
    g_signal_connect (container, "draw", G_CALLBACK (background_info_draw_callback), info);
    gtk_widget_realize (container);
    GdkRGBA color = {0,0,0,0};
    gdk_window_set_background_rgba(gtk_widget_get_window(container), &color);

    return info;
}