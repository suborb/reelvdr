
/*
** compile with

g++  gstreamer_tags.c  `pkg-config --cflags gstreamer-0.10`  `pkg-config --cflags gstreamer-interfaces-0.10` `pkg-config --libs gstreamer-0.10` `pkg-config --libs gstreamer-interfaces-0.10`

*/

#include <gst/gst.h>
#include <gst/interfaces/navigation.h>
#include <unistd.h>		// sleep()

///-------------call backs -----------------------------------------------------
// signal "audio-tags-changed"
void
audio_tags_changed_cb (GstElement * playbin,
		       gint stream_id, gpointer user_data)
{
  g_print ("%s: playbin %p stream [%d] user_data %p\n",
	   __PRETTY_FUNCTION__, playbin, stream_id, user_data);

}

void
video_tags_changed_cb (GstElement * playbin,
		       gint stream_id, gpointer user_data)
{
  g_print ("%s: playbin %p stream [%d] user_data %p\n",
	   __PRETTY_FUNCTION__, playbin, stream_id, user_data);
}

void
subtitle_tags_changed_cb (GstElement * playbin,
			  gint stream_id, gpointer user_data)
{
  g_print ("%s: playbin %p stream [%d] user_data %p\n",
	   __PRETTY_FUNCTION__, playbin, stream_id, user_data);
}				// subtitle_tags_changed_cb

void
foreach_tags (const GstTagList * list, const gchar * tag, gpointer user_data)
{
  gchar *value = NULL;
  g_print ("\t'%s'\n", tag);
  g_print ("\t\tsize:%d", gst_tag_list_get_tag_size (list, tag));
  if (gst_tag_list_get_string (list, tag, &value) == TRUE)
    {
      g_print ("  string:'%s'\n", value);
      g_free (value);
    }
  else
    g_print ("\n");
}


int
main (int argc, char *argv[])
{
  gst_init (NULL, NULL);

  // create player element
  GstElement *player = gst_element_factory_make ("playbin2", NULL);
  //set output plugins here, video/audio XXX

  // get bus of the player
  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (player));
  GstTagList *tagCache = NULL;

  const char *mrl = argv[1];

  gst_element_set_state (player, GST_STATE_NULL);

  g_print ("trying to play %s\n", mrl);

  // set uri of player
  g_object_set (G_OBJECT (player), "uri", mrl, NULL);

  // watch for new audio/video/subtitle tags
  g_signal_connect (player, "video-tags-changed",
		    G_CALLBACK (video_tags_changed_cb), NULL);

  g_signal_connect (player, "audio-tags-changed",
		    G_CALLBACK (audio_tags_changed_cb), NULL);

  g_signal_connect (player, "text-tags-changed",
		    G_CALLBACK (subtitle_tags_changed_cb), NULL);


  // Start playing
  gst_element_set_state (player, GST_STATE_PLAYING);

  while (1)
    {
      GstMessage *msg;
      while (msg = gst_bus_pop (bus))
	{

	  switch (GST_MESSAGE_TYPE (msg))
	    {

	    case GST_MESSAGE_EOS:	// End of stream
	      g_print ("got EOS\n");
	      return 0;
	      break;

	    case GST_MESSAGE_TAG:	//artist, title, album
	      {
		GstTagList *tags = NULL;

		gst_message_parse_tag (msg, &tags);
		g_print ("Got tags from element %s\n",
			 GST_OBJECT_NAME (msg->src));

		gst_tag_list_foreach (tags, foreach_tags, NULL);

		/*
		   gchar *str = gst_tag_list_to_string(tags);
		   g_print("tag: '%s'\n\n", str);
		   g_free(str);
		 */

		//cache||handle tags
		GstTagList *result_tag = gst_tag_list_merge (tagCache,
							     tags,
							     GST_TAG_MERGE_PREPEND);


		if (tagCache)
		  gst_tag_list_free (tagCache);

		tagCache = result_tag;

		gst_tag_list_free (tags);
	      }
	      break;
	    default:
	      break;
	    }

	}

      sleep (1);
    }


  return 0;
}
