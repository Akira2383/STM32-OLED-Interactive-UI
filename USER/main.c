				#include "stm32f10x.h"
				#include "delay.h"
				#include "oled.h"
				#include "ui.h"
				#include "images.h"
				#include "bsp_adc.h"
				#include "key.h"
				#include "timer.h"
				#include "snake.h"
				#include "breakout.h"
				#include "spectrum.h"
				#include "bsp_iwdg.h"

				static u8 adc_samples[ADC_SAMPLE_COUNT];

				int main(void)
				{
						u8 current_state = STATE_BOOT;
						u8 boot_timer = 0;
						u8 menu_cursor = MENU_TERMINAL;
						u8 menu_keys_ready = 0;
									u32 waveform_timer = 0;
									u32 panel_timer = 0;
				
						delay_init();
						OLED_Init();
									ADC_Sampling_Init();
						OLED_Clear();
						Key_Init();
	          Timer_Init();
					
						while(1)
						{
							IWDG_Feed();
								if (current_state == STATE_BOOT)
								{
						      	OLED_Clear();
							      OLED_Refresh();
										delay_ms(500); 
									
										UI_DrawBMP_Fast(0, 0, 128, 64, logo_bmp, 1);    
										delay_ms(1500); 
										
										UI_DrawBMP_Fast(0, 0, 128, 64, logo_bmp, 0);        
										delay_ms(500); 
				
										boot_timer++;
										if (boot_timer >= 1) 
										{
												current_state = STATE_MENU;
												boot_timer = 0;
												menu_cursor = MENU_TERMINAL;
												OLED_Clear(); 
							UI_Draw_Menu(menu_cursor);
							IWDG_Init();
										}

								}
								else if (current_state == STATE_MENU)
								{		           
							  Key_Scan(&key_next);
							  Key_Scan(&key_enter);

							if (!menu_keys_ready)
							{
									Key_GetEvent(&key_next);
									Key_GetEvent(&key_enter);
									if (key_next.state == KEY_IDLE && key_enter.state == KEY_IDLE)
									{
											menu_keys_ready = 1;
									}
							}
							else if (Key_GetEvent(&key_next) == KEY_EVENT_DOWN)
							{
									menu_cursor++;
									if (menu_cursor > MENU_BASE) menu_cursor = MENU_TERMINAL;
									OLED_Clear();
									UI_Draw_Menu(menu_cursor); 
							}

							if (menu_keys_ready && Key_GetEvent(&key_enter) == KEY_EVENT_DOWN)
							{
								 
									if (menu_cursor == MENU_BASE) 
									{
											current_state = STATE_SENSOR_PAGE;
											waveform_timer = Timer_GetMs();
											panel_timer = waveform_timer;
											UI_ClearAudioRegions();
											OLED_Refresh();
									}
									else if (menu_cursor == MENU_TERMINAL)
									{
										Snake_Run();
										current_state = STATE_MENU;
										OLED_Clear();
										UI_Draw_Menu(menu_cursor);
									}
									else if (menu_cursor == MENU_SQUADS)
									{
										Breakout_Run();
										current_state = STATE_MENU;
										OLED_Clear();
										UI_Draw_Menu(menu_cursor);
									}
							}
								}
						else if (current_state == STATE_SENSOR_PAGE)
						{
								u32 now;
								u8 refresh = 0;

								Key_Scan(&key_enter);
								if (Key_GetEvent(&key_enter) == KEY_EVENT_DOWN)
								{
										current_state = STATE_MENU;
										OLED_Clear();
										UI_Draw_Menu(menu_cursor);
										continue;
								}

								now = Timer_GetMs();
								if (ADC_IsBufferReady() &&
									(now - waveform_timer >= 30 || now - panel_timer >= 100))
								{
										ADC_CopySamples(adc_samples);
										if (now - waveform_timer >= 30)
										{
												UI_DrawWaveform(adc_samples);
												waveform_timer = now;
												refresh = 1;
										}
										if (now - panel_timer >= 100)
										{
											UI_DrawLightPanel(adc_samples);
											panel_timer = now;
											refresh = 1;
										}
										if (refresh) OLED_Refresh();
								}
						}
							}
						}
				
