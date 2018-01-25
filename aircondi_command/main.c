#include <stdio.h>
#include <stdlib.h>

int user_min = 0;
int user_max = 0;

void airconditioner_temperature_range_parsing(char temperature_range[]);

int main()
{

    char aircondi_command[10] = "30-0";
    airconditioner_temperature_range_parsing(aircondi_command);

    printf("min: %d" , user_min);
    printf("max: %d" , user_max);

    return 0;
}

void airconditioner_temperature_range_parsing(char temperature_range[]){

    char *text_one;
    char *text_two;

    const char ch[2] = "-";
    char *token;
    token = strtok(temperature_range, ch);
    text_one = token;

    while(token != NULL){
        text_two = token;
        token = strtok(NULL, ch);
    }
    user_min = strtol(text_two , NULL , 10);
    user_max = strtol(text_one , NULL , 10);
    int temp = 0;
    if(user_min > user_max){
        temp = user_max;
        user_max = user_min;
        user_min = temp;
    } else {
        printf("hello\n");
    }

}
