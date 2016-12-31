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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
//#include <curl.h>
//#include <net_connection.h>
/*
#include "Commnucation/Socket.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <dlog.h>
#include <curl.h>
#include <system_info.h>
*/


/* UDP */
int udp_sockfd;
int udp_serverlen;
struct sockaddr_in udp_serveraddr;
struct hostent *udp_server;
char *udp_the_ip = "192.168.1.188";
int udp_portno = 2362;

#define SRV_IP "999.999.999.999"

#define BUFSIZE 1024

static struct view_info {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *img;
	Evas_Coord width;
	Evas_Coord height;
	cairo_t *cairo;
	cairo_surface_t *surface;
	unsigned char *pixels;


	/* Variable for start drawing state */
	int touch_drawing_start;

	/* Variables for path start and end point */
	int cur_x;
	int cur_y;
	int prev_x;
	int prev_y;

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

	/* udp setup */
	init_udp();
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



	/* Add mouse event callbacks */
	evas_object_event_callback_add(s_info.img, EVAS_CALLBACK_MOUSE_DOWN, mouse_down_cb, NULL);
	evas_object_event_callback_add(s_info.img, EVAS_CALLBACK_MOUSE_UP, mouse_up_cb, NULL);
	evas_object_event_callback_add(s_info.img, EVAS_CALLBACK_MOUSE_MOVE, mouse_move_cb, NULL);


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


/* When user touch down on screen, EVAS_CALLBACK_MOUSE_DOWN event occurred
	At that time this mouse_down_cb function callback called
	In this function, can get and set the touched position and
	If this touch down event is first occurred, can change
	touch_drawing_start's state to enable start drawing */
void mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *) event_info;

	/* Change the variable's state to enable start drawing */
	if (s_info.touch_drawing_start == 0)
		s_info.touch_drawing_start = 1;

	/* Get previous position from Evas_Event_Mouse_Down event */
	s_info.prev_x = ev->canvas.x;
	s_info.prev_y = ev->canvas.y;


	print_debug((int)ev->canvas.x,(int)ev->canvas.y);
	send_udp((int)ev->canvas.x,(int)ev->canvas.y);
	//send_udp(1,2);
}


/* When user touch off on screen, EVAS_CALLBACK_MOUSE_UP event occurred
	At that time this mouse_up_cb function callback called
	In this function, get and set the touch end position
	Can draw a line from down event position to up event position */
void mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Up *ev = (Evas_Event_Mouse_Up *) event_info;

	/* Get current position from Evas_Event_Mouse_Up event */
	s_info.cur_x = ev->canvas.x;
	s_info.cur_y = ev->canvas.y;

	send_udp((int)ev->canvas.x,(int)ev->canvas.y);
	//print_debug((int)ev->canvas.x,(int)ev->canvas.y);
}

/* When user touch and move on screen, EVAS_CALLBACK_MOUSE_MOVE event occurred
	At that time this mouse_move_cb function callback called
	In this function, can get the mouse moved from some position to other position
	And set the moved position with inputs */
void mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	Evas_Event_Mouse_Move *ev = (Evas_Event_Mouse_Move *)event_info;

	/* Get touch moved position values */
	s_info.cur_x = ev->cur.canvas.x;
	s_info.cur_y = ev->cur.canvas.y;
	s_info.prev_x = ev->prev.canvas.x;
	s_info.prev_y = ev->prev.canvas.y;

	//print_debug((int)s_info.cur_x, (int)s_info.cur_y);
	send_udp((int)s_info.cur_x, (int)s_info.cur_y);
}

void print_debug(int x, int y)
{
	/* debug location */
	char debug_buff[256];
	snprintf(debug_buff, 256, "x = %d  y = %d", x, y);


	dlog_print(DLOG_ERROR, LOG_TAG, "*******dan test %d", 42);
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, debug_buff);
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
}

void error(char *msg) {

	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, msg);
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
    //exit(0);
}

void eprint(char *msg) {

	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, msg);
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
	dlog_print(DLOG_ERROR, LOG_TAG, "************************************************");
    //exit(0);
}




int send_udp_one(void)
{

    int sockfd, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *the_ip = "192.168.1.188";
    int portno = 2362;
    //CURL *curl;
    //CURLcode res;
    //curl_socket_t sockfd;
//    char buf[BUFSIZE];


    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* build the server's Internet address */
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(the_ip);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    char *buf = "hello robot\n\r";

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    //n = sendto(sockfd, buf, strlen(buf), MSG_DONTWAIT, &serveraddr, serverlen);

    if (n < 0)
      error("ERROR in sendto");


    /* print the server's reply */
/*
    n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
    if (n < 0)
      error("ERROR in recvfrom");
    printf("Echo from server: %s", buf);
 */
    return 0;


}




int init_udp(void)
{
    /* socket: create the socket */
    udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sockfd < 0)
    {
        error("ERROR opening socket");
        return -1;
    }

    /* build the server's Internet address */
    udp_serveraddr.sin_family = AF_INET;
    udp_serveraddr.sin_addr.s_addr = inet_addr(udp_the_ip);
    udp_serveraddr.sin_port = htons(udp_portno);

    udp_serverlen = sizeof(udp_serveraddr);

    return 0;

}

int send_udp(int x, int y)
{
    /* get a message from the user */
    char buf[256];

	char *s1 = "joy left";
	double k = 1.4;
	double xf = (x/360.0 * 2.0 - 1.0) * k;
	double yf = -(y/360.0 * 2.0 - 1.0) * k;

	if(xf >  1.0) xf = 1.0;
	if(xf < -1.0) xf = -1.0;
	if(yf >  1.0) yf = 1.0;
	if(yf < -1.0) yf = -1.0;
	snprintf(buf, 256, "%s %.3f %.3f", s1, xf, yf);


    /* send the message to the server */
    int n = sendto(udp_sockfd, buf, strlen(buf), 0, &udp_serveraddr, udp_serverlen);
//    n = sendto(sockfd, buf, strlen(buf), MSG_DONTWAIT, &serveraddr, serverlen);

    if (n < 0)
      error("ERROR in sendto");

    return 0;


}




