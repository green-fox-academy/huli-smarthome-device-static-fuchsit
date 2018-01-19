#include <stdio.h>
#include <stdlib.h>

void json_hexa_for_rgbled(char led_color_hexa[]);

int main()
{

    char led_color[] = "FF80FF";

    json_hexa_for_rgbled(led_color);

    return 0;
}

void json_hexa_for_rgbled(char led_color_hexa[]){
    //char led_color[] = led_color_hexa;

    int red = 0;
	int blue = 0;
	int green = 0;

	char red_text[5] = "0x";
	char blue_text[5] = "0x";
	char green_text[5] = "0x";

    int size = strlen(led_color_hexa);

	for (int i = 0 ; i <  size ; ++i){

        if (i < 2){
            red_text[i + 2] = led_color_hexa[i];
         }
        if ( i > 1 && i < 4){
            blue_text [i] = led_color_hexa[i];
        }
        if ( i> 3 && i < 6){
            green_text [i-2] = led_color_hexa[i];
        }
	}
    red = strtol(red_text , NULL , 16);
    blue = strtol(blue_text , NULL , 16);
    green = strtol(green_text , NULL , 16);
    printf("hexared: %d" , red);
    printf(" hexablue: %d" , blue);
    printf(" hexagreen: %d" , green);
}
