#include <assert.h>
#include <gst/gst.h>
#include <string.h>

struct MyData
{
    GstElement *playbin;
    int idx;
    int count;
    char **files;
};

char abs_path[PATH_MAX + 1];

static void on_about_to_finish(GstElement *obj, gpointer userdata)
{
    struct MyData *data = (struct MyData *)userdata;

    if (data->idx == data->count - 1) {
        exit(0);
    }

    const char *file = data->files[data->idx];
    ++data->idx;

    assert(realpath(file, abs_path));
    gchar *uri = g_filename_to_uri(abs_path, NULL, NULL);

    g_print("about-to-finish, setting next: %s\n", file);
    g_object_set(G_OBJECT(data->playbin), "uri", uri, NULL);
    g_free(uri);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        g_printerr("usage: %s FILE1 FILE2 ...\n", argv[0]);
        return -1;
    }

    gst_init(&argc, &argv);

    GstElement *playbin = gst_element_factory_make("playbin3", NULL);
    assert(playbin);

    {
        GstElement *pitch = gst_element_factory_make("pitch", NULL);
        assert(pitch != NULL);

        GstElement *sink = gst_element_factory_make("autoaudiosink", NULL);
        GstElement *bin = gst_bin_new(NULL);
        gst_bin_add_many(GST_BIN(bin), pitch, sink, NULL);
        gst_element_link_many(pitch, sink, NULL);

        GstPad *pad = gst_element_get_static_pad(pitch, "sink");
        gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);

        g_object_set(playbin, "audio-sink", bin, NULL);

        g_object_set(pitch, "pitch", 2.0, NULL);
    }

    struct MyData data;
    data.playbin = playbin;
    data.idx = 0;
    data.files = &(argv[1]);
    data.count = argc - 1;
    g_signal_connect(playbin, "about-to-finish", G_CALLBACK(on_about_to_finish),
                     &data);

    const char *file = data.files[data.idx];
    ++data.idx;

    assert(realpath(file, abs_path));
    gchar *uri = g_filename_to_uri(abs_path, NULL, NULL);
    g_object_set(G_OBJECT(playbin), "uri", uri, NULL);
    g_free(uri);

    g_print("playing: %s\n", data.files[0]);
    gst_element_set_state(playbin, GST_STATE_PLAYING);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}
