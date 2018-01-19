#include <stdio.h>
#include <stdlib.h>

//void json_hexa_for_rgbled(char led_color_hexa[]);

int red_led(char led_color_hex[]);
int green_led(char led_color_hex[]);
int blue_led(char led_color_hex[]);

int main()
{

    char led_color[] = "FF80FF";

    printf("red: %d" , red_led(led_color));
    printf("green: %d" , green_led(led_color));
    printf("blue: %d" , blue_led(led_color));

//    json_hexa_for_rgbled(led_color);

    return 0;
}
int red_led(char led_color_hex[]){

    int red = 0;
    char red_text[2];
    red_text[0] = led_color_hex[0];
    red_text[1] = led_color_hex[1];
    red = strtol(red_text , NULL , 16);

    return red;
}
int green_led(char led_color_hex[]){

    int green = 0;
    char green_text[2];
    green_text[0] = led_color_hex[2];
    green_text[1] = led_color_hex[3];
    green = strtol(green_text , NULL , 16);

    return green;
}
int blue_led(char led_color_hex[]){

    int blue = 0;
    char blue_text[2];
    blue_text[0] = led_color_hex[4];
    blue_text[1] = led_color_hex[5];
    blue = strtol(blue_text , NULL , 16);

    return blue;
}

/*
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
*/
