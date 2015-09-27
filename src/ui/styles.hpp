#pragma once
#include "ui_base_classes.hpp"

const unsigned char c_ui_normal_color[]= { 128, 128, 128, 255 };
const unsigned char c_ui_active_color[]= { 200, 48, 48, 255 };
const unsigned char c_ui_text_color[]= { 96, 96, 96, 255 };
const unsigned char c_ui_active_text_color[]= { 150, 32, 32, 255 };

const ui_Style c_ui_main_style(
	c_ui_normal_color,
	c_ui_active_color,
	ui_Style::TextAlignment::Center,
	c_ui_text_color,
	c_ui_active_text_color );

const ui_Style c_ui_texts_style(
	c_ui_normal_color,
	c_ui_active_color,
	ui_Style::TextAlignment::Center,
	c_ui_text_color,
	c_ui_text_color );
