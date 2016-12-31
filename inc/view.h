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

#if !defined(_VIEW_H)
#define _VIEW_H

#define GRP_MAIN "main"

/*
 * Create a view
 */
void view_create(void);
Evas_Object *view_create_win(const char *pkg_name);
void view_destroy(void);
void win_delete_request_cb(void *data, Evas_Object *obj, void *event_info);
void win_back_cb(void *data, Evas_Object *obj, void *event_info);
void win_resize_cb(void *data, Evas *e , Evas_Object *obj , void *event_info);
void start_cairo_drawing(void);
void mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
void mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
void mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
void print_debug(int x, int y);
int send_udp_one(void);
void eprint(char *msg);
int send_udp(int x, int y);
int init_udp(void);
#endif
