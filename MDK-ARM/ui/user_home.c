#include "user_home.h"

lv_subject_t temp_subject;

// (1)observer参数 : 同一个回调函数可以应用到多个观察者 通过第一个参数的地址来表示当前触发回调的是哪一个观察者
// (2)subject参数 : 添加观察者的主题 => 对应观察的对象
void temp_obs_cb(lv_observer_t *observer, lv_subject_t *subject)
{
    int32_t pre_value = lv_subject_get_previous_int(subject);
    int32_t cur_value = lv_subject_get_int(subject);
    printf(" pre_value: %d, cur_value: %d\n", pre_value, cur_value);
}

// 按下之后添加温度值
void btn1_event_cb(lv_event_t *e)
{
    // 获取主题
    lv_subject_t *subject = lv_event_get_user_data(e);
    // 修改主题的值 => +1
    lv_subject_set_int(subject, lv_subject_get_int(subject) + 1);
}

// 按下之后减少温度值
void btn2_event_cb(lv_event_t *e)
{
    // 获取主题
    lv_subject_t *subject = lv_event_get_user_data(e);
    // 修改主题的值 => -1
    lv_subject_set_int(subject, lv_subject_get_int(subject) - 1);
}

// 用于翻译的数组
static const char *const languages[] = {"en", "chinese", NULL};
static const char *const tags[] = {"obs", "translation", NULL};
static const char *const translations[] = {
    // 字符串只能写成UTF-8的格式 不然LVGL没法识别
    // keil中的编译器使用的GBK  会出现不定期的乱码  
    "obs", " 观察者 ",
    "translation", "翻译"};

// 按下之后减少温度值
void dropdown_event_cb(lv_event_t *e)
{
    // 1. 获取下拉菜单组件
    lv_obj_t *dropdown = lv_event_get_user_data(e);
    // 2. 获取当前的选项下标
    int32_t selected_index = lv_dropdown_get_selected(dropdown);
    printf("selected_index: %d\n", selected_index);
    if (selected_index == 0)
    {
        lv_translation_set_language("en");
    }
    else if (selected_index == 1)
    {
        lv_translation_set_language("chinese");
    }
}

void language_change_event_cb(lv_event_t *e)
{
    lv_obj_t *label = lv_event_get_target_obj(e);
    char *tag = lv_event_get_user_data(e);
    lv_label_set_text(label, lv_tr(tag));
}

void user_home_create(void)
{
    // 开启静态翻译功能
    lv_translation_add_static(languages, tags, translations);

    lv_translation_set_language("en");

    // 1. 创建一个标签视图
    lv_obj_t *tabview = lv_tabview_create(lv_screen_active());
    // 1.1 设置标签视图的样式
    lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));

    // 1.2 创建一个标签页
    lv_obj_t *obs_table = lv_tabview_add_tab(tabview, lv_tr("obs"));
    lv_obj_t *translation_table = lv_tabview_add_tab(tabview, lv_tr("translation"));

    // 添加接收翻译修改的事件回调函数
    // (1) 获取table bar
    lv_obj_t *table_bar = lv_tabview_get_tab_bar(tabview);
    // (2) 循环调用 多个按钮
    for (uint8_t i = 0; i < 2; i++)
    {
        lv_obj_t *btn = lv_obj_get_child(table_bar, i);
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        // 添加语言变更的回调
        lv_obj_add_event_cb(label, language_change_event_cb, LV_EVENT_TRANSLATION_LANGUAGE_CHANGED, (void *)tags[i]);
    }

    // 2. 添加对应的组件到obs页面
    // 2.1 外部参数的观察者模式
    // 2.1.1 初始化一个主题  => 选择正确的类型和初始值
    lv_subject_init_int(&temp_subject, 10);
    // 2.1.2 添加外部参数的观察者
    lv_observer_t *obs_cb = lv_subject_add_observer(&temp_subject, temp_obs_cb, NULL);

    // 2.2 创建按钮 => 调整温度
    lv_obj_t *btn1 = lv_btn_create(obs_table);
    lv_obj_set_size(btn1, 50, 50);
    lv_obj_align(btn1, LV_ALIGN_CENTER, -50, 0);
    lv_obj_add_event_cb(btn1, btn1_event_cb, LV_EVENT_CLICKED, &temp_subject);

    // 2.2.1 创建标签 => 显示加号
    lv_obj_t *label = lv_label_create(btn1);
    lv_label_set_text(label, LV_SYMBOL_PLUS);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // 按钮2
    lv_obj_t *btn2 = lv_btn_create(obs_table);
    lv_obj_set_size(btn2, 50, 50);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 50, 0);
    lv_obj_add_event_cb(btn2, btn2_event_cb, LV_EVENT_CLICKED, &temp_subject);
    lv_obj_t *label2 = lv_label_create(btn2);
    lv_label_set_text(label2, LV_SYMBOL_MINUS);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, 0);

    // 2.3 创建标签 => 显示当前的温度值
    lv_obj_t *label3 = lv_label_create(obs_table);
    // lv_label_set_text_fmt(label3, "temp: %d", lv_subject_get_int(&temp_subject));
    // 需要使用观察者绑定组件的方式 => 同步修改温度的值
    lv_label_bind_text(label3, &temp_subject, "temp: %d");
    lv_obj_align(label3, LV_ALIGN_TOP_MID, 0, 50);

    // 2.4 创建滑块 => 实现双向绑定
    lv_obj_t *slider = lv_slider_create(obs_table);
    lv_obj_set_size(slider, 200, 20);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_slider_set_range(slider, -100, 100);
    lv_slider_bind_value(slider, &temp_subject);

    // 3. 在翻译页面中添加对应的组件 切换语言
    lv_obj_t *dropdown = lv_dropdown_create(translation_table);

    lv_dropdown_set_options(dropdown, "en\n"
                                      "中文");
    // 添加回调函数
    lv_obj_add_event_cb(dropdown, dropdown_event_cb, LV_EVENT_VALUE_CHANGED, dropdown);
}
