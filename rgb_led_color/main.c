#include "rgb_led_color.h"

//int red_led(char led_color_hex[]);
//int green_led(char led_color_hex[]);
//int blue_led(char led_color_hex[]);

int main()
{
    printf("red: %d, green: %d, blue: %d\n" , red, green , blue);

    char led_color[] = "FF80FF";

    json_hexa_for_rgbled(led_color);

    printf("red: %d, green: %d, blue: %d" , red, green , blue);
}
/*


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
*/




