/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "wearable-robot-controller.h"
#include "view.h"

static struct view_info {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *img;
	Evas_Coord width;
	Evas_Coord height;
	cairo_t *cairo;
	cairo_surface_t *surface;
	unsigned char *pixels;
} s_info = {
	.win = NULL,
	.conform = NULL,
};

/*
 * @brief Create Essential Object window, conformant and layout
 */
void view_create(void)
{
	/* Create window */
	s_info.win = view_create_win(PACKAGE);
	if (s_info.win == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to create a window.");
		return;
	}

	/* Show window after main view is set up */
	evas_object_show(s_info.win);
}

/*
 * @brief Make a basic window named package
 * @param[in] pkg_name Name of the window
 */
Evas_Object *view_create_win(const char *pkg_name)
{
	Evas_Object *win = NULL;

	/*
	 * Window
	 * Create and initialize elm_win.
	 * elm_win is mandatory to manipulate window
	 */
	win = elm_win_util_standard_add(pkg_name, pkg_name);
	elm_win_conformant_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);

	/* Rotation setting */
	if (elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(win, EEXT_CALLBACK_BACK, win_back_cb, NULL);
	evas_object_event_callback_add(win, EVAS_CALLBACK_RESIZE, win_resize_cb, NULL);
	evas_object_show(win);

	/* Create image */
	s_info.img = evas_object_image_filled_add(evas_object_evas_get(win));
	evas_object_show(s_info.img);

	return win;
}

/*
 * @brief Destroy window and free important data to finish this application
 */
void view_destroy(void)
{
	if (s_info.win == NULL)
		return;

	/* Destroy cairo surface and device */
	cairo_surface_destroy(s_info.surface);
	cairo_destroy(s_info.cairo);

	evas_object_del(s_info.win);
}

void win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	view_destroy();
	ui_app_exit();
}

void win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	/* Let window go to hide state. */
	elm_win_lower(s_info.win);
}

void win_resize_cb(void *data, Evas *e , Evas_Object *obj , void *event_info)
{
	/* When window resize event occurred
		Check first cairo surface already exist
		If cairo surface exist, destroy it */
	if (s_info.surface) {
		/* Destroy previous cairo canvas */
		cairo_surface_destroy(s_info.surface);
		cairo_destroy(s_info.cairo);
		s_info.surface = NULL;
		s_info.cairo = NULL;
	}

	/* When window resize event occurred
		If no cairo surface exist or destroyed
		Create cairo surface with resized Window size */
	if (!s_info.surface) {
		/* Get screen size */
		evas_object_geometry_get(obj, NULL, NULL, &s_info.width, &s_info.height);

		/* Set image size */
		evas_object_image_size_set(s_info.img, s_info.width, s_info.height);
		evas_object_resize(s_info.img, s_info.width, s_info.height);
		evas_object_show(s_info.img);

		/* Create new cairo canvas for resized window */
		s_info.pixels = (unsigned char*)evas_object_image_data_get(s_info.img, 1);
		s_info.surface = cairo_image_surface_create_for_data(s_info.pixels,
						CAIRO_FORMAT_ARGB32, s_info.width, s_info.height, s_info.width * 4);
		s_info.cairo = cairo_create(s_info.surface);

		/* Get image data from png file and paint as default background */
		start_cairo_drawing();
	}
}

/* In this function, first get path a png file as a resource
	After get image data from png file, use the data as source surface
	This source could be painted on cairo context and displayed */
void start_cairo_drawing(void)
{
	/* Get the sample stored png image file path to use as resource */
	char sample_filepath[256];
	char *source_filename = "wrc-back-r1.png";
	char *resource_path = app_get_resource_path();
	snprintf(sample_filepath, 256, "%s/%s", resource_path, source_filename);
	free(resource_path);

	/* Get image data from png file as image source surface */
	cairo_surface_t *image = cairo_image_surface_create_from_png(sample_filepath);
	cairo_set_source_surface(s_info.cairo, image, 0, 0);

	/* Use operator as CAIRO_OPERATOR_SOURCE to enable to use image source */
	cairo_set_operator(s_info.cairo, CAIRO_OPERATOR_SOURCE);

	/* Paint image source */
	cairo_paint(s_info.cairo);

	/* Destroy the image source */
	cairo_surface_destroy(image);

	/* Render stacked cairo APIs on cairo context's surface */
	cairo_surface_flush(s_info.surface);

	/* Display this cairo image painting on screen */
	evas_object_image_data_update_add(s_info.img, 0, 0, s_info.width, s_info.height);
}
