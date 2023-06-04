#include <zephyr.h>
#include <lvgl.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024
/* scheduling priority used by each thread */
#define PRIORITY 7

K_THREAD_STACK_DEFINE(threadA_stack_area, STACKSIZE);
static struct k_thread threadA_data;

K_THREAD_STACK_DEFINE(threadB_stack_area, STACKSIZE);
static struct k_thread threadB_data;

void label_move(char *text, lv_coord_t sx, lv_coord_t sy, int8_t sdx, int8_t sdy, uint32_t sleep_time)
{
	lv_coord_t x = sx;
	lv_coord_t y = sy;

	int8_t dx = sdx;
	int8_t dy = sdy;

	lv_obj_t *label = lv_label_create(lv_scr_act());
	lv_label_set_text(label, text);

	while (1) {

		lv_obj_set_pos(label, x, y);

		if ((int16_t)x + dx < 0 || (int16_t)x + dx > (LV_HOR_RES - lv_obj_get_width(label)))
			dx = -dx;
		if ((int16_t)y + dy < 0 || (int16_t)y + dy > (LV_VER_RES - lv_obj_get_height(label)))
			dy = -dy;

		x += dx;
		y += dy;

		k_sleep(K_MSEC(sleep_time));
	}
}

void threadA(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	label_move("hello", 0, 0, 1, 1, 150);
}

void threadB(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	label_move("world", 0, 200, -1, -1, 150);
}

void start_demo(void)
{
	k_thread_create(&threadA_data, threadA_stack_area, K_THREAD_STACK_SIZEOF(threadA_stack_area), threadA, NULL, NULL, NULL, PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&threadA_data, "thread_a");

	k_thread_create(&threadB_data, threadB_stack_area, K_THREAD_STACK_SIZEOF(threadB_stack_area), threadB, NULL, NULL, NULL, PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&threadB_data, "thread_b");

	k_thread_start(&threadA_data);
	k_thread_start(&threadB_data);
}
