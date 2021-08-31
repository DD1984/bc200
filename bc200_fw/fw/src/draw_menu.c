#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <lvgl.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(menu);

const char *items[] = {
	"item1",
	"item2",
	"item3",
	"item4",
	"item5",
	"item6",
};

static lv_style_t menu_btn_style;

void menu_btn_event_cb(lv_obj_t * obj, lv_event_t event)
{
	if (event == LV_EVENT_PRESSED)
		LOG_INF("btn event pressed");
}

void draw_menu(void)
{
	lv_obj_t *list = lv_list_create(lv_scr_act(), NULL);
	lv_obj_set_width(list, LV_HOR_RES);
	lv_obj_set_height(list, LV_VER_RES);

	lv_group_t * group = lv_group_create();
	lv_group_add_obj(group, list);

	const struct device *poll_btn_dev = device_get_binding("POLL_BTNS");
	lv_indev_t *indev = (lv_indev_t *)*(size_t *)(poll_btn_dev->data);
	lv_indev_set_group(indev, group);

	lv_style_init(&menu_btn_style);
	lv_style_set_border_width(&menu_btn_style, LV_STATE_DEFAULT, 0);

	lv_style_set_border_width(&menu_btn_style, LV_STATE_FOCUSED, 1);
	lv_style_set_border_side(&menu_btn_style, LV_STATE_FOCUSED, LV_BORDER_SIDE_FULL);

	lv_style_set_text_decor(&menu_btn_style, LV_STATE_FOCUSED, LV_TEXT_DECOR_NONE);

	for (unsigned int i = 0; i < ARRAY_SIZE(items); i++) {
		lv_obj_t *btn = lv_list_add_btn(list, NULL, items[i]);
		lv_obj_add_style(btn, LV_BTN_PART_MAIN, &menu_btn_style);

		lv_obj_set_event_cb(btn, menu_btn_event_cb); 
	}
}
