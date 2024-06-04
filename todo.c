#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_image.h"
#include "stdbool.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define FONT_SIZE 70
#define SUBTEXT_FONT_SIZE FONT_SIZE-10

#define DATA_PATH "./todo.bin"

typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH
} Task_Priority;

typedef enum {
    TAB_DASHBOARD = 0,
    TAB_NEW_TASK
} gui_tab;

typedef struct {
    SDL_Rect rect;
    struct Color {
        Uint8 r, g, b, a;
    } colour;
    bool pressed;
} Button;

typedef struct {
    Task_Priority priority;
    char *desc;
    char *due_date;
    bool completed;
} Task_entry;


// save ToDo entries to .bin file
static void serialize_todo_entry(FILE *file, Task_entry *entry)
{
    fwrite(&entry->completed, sizeof(bool), 1, file);

    size_t desc_len = strlen(entry->desc) + 1;
    fwrite(&desc_len, sizeof(size_t), 1, file);
    fwrite(entry->desc, sizeof(char), desc_len, file);

    size_t due_date_len = strlen(entry->due_date) + 1;
    fwrite(&due_date_len, sizeof(size_t), 1, file);
    fwrite(entry->due_date, sizeof(char), due_date_len, file);

    fwrite(&entry->priority, sizeof(Task_Priority), 1, file);
}

static void serialize_todo_list(const char *filename, Task_entry *entries[], uint32_t numentries)
{
    FILE *file = fopen(filename, "wb");
    if(!file) {
        printf("Failed to open File: %s", filename);
        return;
    }

    for(uint32_t i = 0; i < numentries; i++) {
        serialize_todo_entry(file, entries[i]);
    }
}

Task_entry *deserialize_todo_entry(FILE *file)
{
    Task_entry *entry = (Task_entry*)malloc(sizeof(Task_entry));

    if(fread(&entry->completed, sizeof(bool), 1, file) != 1) {
        free(entry);
        return NULL;
    }

    size_t desc_len;
    if(fread(&desc_len, sizeof(size_t), 1, file) != 1) {
        free(entry);
        return NULL;
    }

    entry->desc = (char*)malloc(desc_len);
    if(fread(entry->desc, sizeof(char), desc_len, file) != desc_len) {
        free(entry->desc);
        free(entry);
        return NULL;
    }

    size_t due_date_len;
    fread(&due_date_len, sizeof(size_t), 1, file);

    entry->due_date = (char*)malloc(due_date_len);
    if(fread(entry->due_date, sizeof(char), due_date_len, file) != due_date_len) {
        free(entry->desc);
        free(entry->due_date);
        free(entry);
        return NULL;
    }

    if(fread(&entry->priority, sizeof(Task_Priority), 1, file) != 1) {
        free(entry->desc);
        free(entry->due_date);
        free(entry);
        return NULL;
    }

    return entry;
}

void deserialize_todo_list(const char *filename, Task_entry **entries, uint32_t *numentries)
{
    FILE *file = fopen(filename, "rb");
    if(!file) {
        printf("Failed to open File: %s", filename);
        return;
    }

    Task_entry *entry;
    while((entry = deserialize_todo_entry(file)) != NULL) {
        entries[(*numentries)++] = entry;
    }

    fclose(file);
}

// save text texture
void get_text_and_rect(SDL_Renderer *renderer, int x, int y, char *text,
                       TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect, SDL_Color textColor)
{
    int text_width;
    int text_height;
    SDL_Surface *surfaceMessage;

    surfaceMessage = TTF_RenderText_Solid(font, text, textColor);
    *texture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
    text_width = surfaceMessage->w;
    text_height = surfaceMessage->h;
    SDL_FreeSurface(surfaceMessage);
    rect->x = x;
    rect->y = y;
    rect->w = text_width;
    rect->h = text_height;
}

void button_process_event(Button *button, const SDL_Event *e)
{
    if(e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        if(e->button.x >= button->rect.x && e->button.x <= (button->rect.x + button->rect.w) &&
           e->button.y >= button->rect.y && e->button.y <= (button->rect.y + button->rect.h)) {
            button->pressed = true;
        }
    }
}

bool button(SDL_Renderer *renderer, Button *button, SDL_Texture *text_texture, SDL_Rect *text_rect)
{
    // draw Button
    SDL_SetRenderDrawColor(renderer, button->colour.r, button->colour.g, button->colour.b, button->colour.a);
    SDL_RenderFillRect(renderer, &(button->rect));

    if(text_texture != NULL && text_rect != NULL)
        SDL_RenderCopy(renderer, text_texture, NULL, text_rect);

    if(button->pressed) {
        button->pressed = false;
        return true;
    }

    return false;
}

void delete_entry(Task_entry **entries, uint32_t *numentries, uint32_t delete_index)
{
    for(uint32_t i = delete_index; i < *numentries; i++) {
        entries[i] = entries[i+1];
    }

    (*numentries)--;
}

static void draw_entries(SDL_Renderer *renderer, TTF_Font *font, Task_entry *entries[], uint32_t numentries, SDL_Texture *trash_icon_texture, Button *trash_buttons)
{
    uint32_t y = 100;
    SDL_Texture *entry_texture;
    SDL_Rect entry_rect;
    SDL_Rect trash_icon_rect;

    for(uint32_t i = 0; i < numentries; i++) {

        // draw trash icon
        trash_icon_rect = (SDL_Rect) {
            .x = 50, .y = y, .w = 40, .h = 40
        };
        trash_buttons[i] = (Button) {
            .rect = trash_icon_rect, .colour = {25, 25, 25, 255}, .pressed = trash_buttons[i].pressed
        };

        TTF_SetFontSize(font, FONT_SIZE);

        get_text_and_rect(renderer, 100, y, (char*)entries[i]->desc, font, &entry_texture, &entry_rect, (SDL_Color) {
            230, 230, 230, 230
        });

        entry_rect.w /= 2;
        entry_rect.h /= 2;

        SDL_RenderCopy(renderer, entry_texture, NULL, &entry_rect);

        get_text_and_rect(renderer, 100, y + 30, (char*)entries[i]->due_date, font, &entry_texture, &entry_rect, (SDL_Color) {
            230, 230, 230, 230
        });

        entry_rect.y += 10;
        entry_rect.x += 5;
        entry_rect.w /= 3;
        entry_rect.h /= 3;

        SDL_RenderCopy(renderer, entry_texture, NULL, &entry_rect);

        y += 80;
    }

}

Task_entry *create_entry(char *desciption, Task_Priority priority, char *due_date, bool completed)
{
    Task_entry *entry = malloc(sizeof(Task_entry));

    char *desc_buffer = (char*)malloc(strlen(desciption) + 1);
    strcpy(desc_buffer, desciption);
    entry->desc = desc_buffer;
    entry->priority = priority;

    char *due_date_buffer = (char*)malloc(strlen(due_date) + 1);
    strcpy(due_date_buffer, due_date);
    entry->due_date = due_date_buffer;
    entry->completed = completed;

    return entry;
}

void entries_push(Task_entry **entries, uint32_t *numentries, char *desc, Task_Priority priority, char *due_date, bool completed)
{
    entries[(*numentries)++] = create_entry(desc, priority, due_date, completed);
    serialize_todo_list(DATA_PATH, entries, *numentries);
}

int main(int argc, char* argv[])
{
    static gui_tab current_tab;
    static Task_entry *entries[1024];
    static uint32_t numentries = 0;
    static struct Color gui_purple = {
        .r = 110, .g = 86, .b = 175, .a = 255
    };

    // get saved Data
    deserialize_todo_list(DATA_PATH, entries, &numentries);

    SDL_Rect title_rec, new_task_button_text_rect, entry_text_input_button_rect, entry_text_input_button_text_rect, new_task_back_button_rect, new_task_add_text_rect, new_task_add_button_rect;
    SDL_Texture *title_texture, *new_task_button_texture, *new_task_button_text_texture, *entry_text_input_button_text_texture, *trash_icon_texture, *new_task_back_button_texture, *new_task_add_text_texture;

    SDL_Rect button_rect = {.x = WINDOW_WIDTH - (WINDOW_WIDTH /4), .y = 10, .h = 40, .w = 200};
    Button new_task_button = {.rect = button_rect, .colour = gui_purple};

    entry_text_input_button_rect = (SDL_Rect) {
        .x = WINDOW_WIDTH / 4, .y = 100, .h = 40, .w = 450
    };
    Button entry_text_input_button = {.rect = entry_text_input_button_rect, .colour = { .r = 25, .g = 25, .b = 25, .a = 0}};
    char entry_text_input[1024];
    strncat(entry_text_input, "Enter Task Description", strlen("Enter Task Description") + 1);

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );
    SDL_StopTextInput();

    SDL_Window *window = SDL_CreateWindow("ToDo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);

    TTF_Init();
    // /usr/share/fonts/corefonts/arial.ttf
    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/nerd-font/HackNerdFontMono-Bold.ttf", FONT_SIZE);
    if (font == NULL) {
        fprintf(stderr, "error: font not found\n");
        exit(EXIT_FAILURE);
    }
    // create title Text Texture
    get_text_and_rect(renderer, WINDOW_WIDTH / 4, 15, "ToDo List", font, &title_texture, &title_rec, (SDL_Color) {
        255, 255, 255, 0
    });

    // create new Task Text Button Texture
    get_text_and_rect(renderer, new_task_button.rect.x + 25, new_task_button.rect.y + 5, "new Task", font, &new_task_button_text_texture, &new_task_button_text_rect, (SDL_Color) {
        25, 25, 25, 255
    });
    new_task_button_text_rect.w /= 2;
    new_task_button_text_rect.h /= 2;

    trash_icon_texture = IMG_LoadTexture(renderer, "./icons/trash-bin.png");
    Button trash_buttons[1024];

    new_task_back_button_texture = IMG_LoadTexture(renderer, "./icons/left.png");
    new_task_back_button_rect = (SDL_Rect) {
        .x = 60, .y = WINDOW_HEIGHT - 80, .w = 60, .h = 40
    };
    Button new_task_back_button = {.rect = new_task_back_button_rect, .colour = gui_purple};


    get_text_and_rect(renderer, WINDOW_WIDTH - 120, WINDOW_HEIGHT - 80, "ADD", font, &new_task_add_text_texture, &new_task_add_text_rect, (SDL_Color) {
        25,25,25,255
    });
    new_task_add_text_rect.w /= 2;
    new_task_add_text_rect.h /= 2;

    new_task_add_button_rect = (SDL_Rect) {
        .x = new_task_add_text_rect.x - 10, .y = new_task_add_text_rect.y, .w = new_task_add_text_rect.w + 20, .h = new_task_add_text_rect.h
    };
    Button new_task_add_button = {.rect = new_task_add_button_rect, .colour = gui_purple};

    int quit = 0;
    SDL_Event sdl_event;
    while(!quit) {
        // Start of SDL Event loop
        // loop to check if exit button is clicked
        while(SDL_PollEvent(&sdl_event) > 0) {
            switch(sdl_event.type) {
            case SDL_QUIT:
                serialize_todo_list(DATA_PATH, entries, numentries);
                quit = 1;
                break;

            case SDL_MOUSEBUTTONDOWN:
                button_process_event(&new_task_button, &sdl_event);
                button_process_event(&entry_text_input_button, &sdl_event);
                for(uint32_t i = 0; i < numentries; i++) {
                    button_process_event(&(trash_buttons[i]), &sdl_event);
                }
                button_process_event(&new_task_back_button, &sdl_event);
                button_process_event(&new_task_add_button, &sdl_event);
                break;

            // get user text input
            case SDL_TEXTINPUT:
                if(sdl_event.key.keysym.sym != SDLK_RETURN)
                    strcat(entry_text_input, sdl_event.text.text);
                break;

            case SDL_KEYDOWN:
                // check if key pressed is ESC and quit
                if (sdl_event.key.keysym.sym == SDLK_ESCAPE) {
                    switch(current_tab) {
                    case TAB_DASHBOARD:
                        sdl_event.type = SDL_QUIT;
                        SDL_PushEvent(&sdl_event);
                        break;
                    case TAB_NEW_TASK:
                        SDL_StopTextInput();
                        // empty user text input string
                        memset(&entry_text_input[0], 0, 1024);
                        current_tab = TAB_DASHBOARD;
                        break;
                    }
                } else if(sdl_event.key.keysym.sym == SDLK_RETURN) {
                    SDL_StopTextInput();
                    entries_push(entries, &numentries, &entry_text_input[0], PRIORITY_LOW, " ", false);

                    // empty user text input string
                    memset(&entry_text_input[0], 0, 1024);
                    strncat(entry_text_input, "Enter Task Description", strlen("Enter Task Description") + 1);
                } else if(sdl_event.key.keysym.sym == SDLK_BACKSPACE && strlen(entry_text_input) > 0) {
                    entry_text_input[strlen(entry_text_input) - 1] = '\0';
                }
            }
        }
        // end of SDL Event Loop

        // Select the color for drawing. It is set to dark grey here.
        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 1.0);

        // Clear the entire screen to our selected color.
        SDL_RenderClear(renderer);

        switch(current_tab) {
        case TAB_DASHBOARD: {
            SDL_RenderCopy(renderer, title_texture, NULL, &title_rec);
            draw_entries(renderer, font, entries, numentries, trash_icon_texture, trash_buttons);
            // draw button and check if pressed
            if(button(renderer, &new_task_button, new_task_button_text_texture, &new_task_button_text_rect)) {
                printf("new Task button pressed\n");
                current_tab = TAB_NEW_TASK;
            }

            // check if remove button for Task was pressed
            for(int i = 0; i < numentries; i++) {
                if(button(renderer, trash_buttons + i, trash_icon_texture, &(trash_buttons[i].rect))) {
                    delete_entry(entries, &numentries, i);
                    serialize_todo_list(DATA_PATH, entries, numentries);
                }
            }
            break;
        }
        case TAB_NEW_TASK: {
            // make button outline
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
            SDL_Rect entry_text_input_button_outline_rect = {.x = entry_text_input_button_rect.x - 10, .y = entry_text_input_button_rect.y - 10, .w = entry_text_input_button_text_rect.w + 50, .h = entry_text_input_button_text_rect.h + 20};
            SDL_RenderDrawRect(renderer, &entry_text_input_button_outline_rect);

            if(button(renderer, &entry_text_input_button, NULL, NULL)) {
                printf("entry text input button pressed\n");
                memset(&entry_text_input[0], 0, 1024);
                SDL_StartTextInput();
            } else if(button(renderer, &new_task_back_button, new_task_back_button_texture, &new_task_back_button_rect)) {
                printf("New Task back button pressed\n");
                current_tab = TAB_DASHBOARD;
            } else if(button(renderer, &new_task_add_button, new_task_add_text_texture, &new_task_add_text_rect)) {
                printf("new Task add button pressed\n");
                SDL_StopTextInput();
                entries_push(entries, &numentries, &entry_text_input[0], PRIORITY_LOW, " ", false);

                // empty user text input string
                memset(&entry_text_input[0], 0, 1024);
                strncat(entry_text_input, "Enter Task Description", strlen("Enter Task Description") + 1);
            }
            if(strlen(entry_text_input) > 0) {
                get_text_and_rect(renderer, entry_text_input_button_rect.x + 20, entry_text_input_button_rect.y + 2, &entry_text_input[0], font, &entry_text_input_button_text_texture, &entry_text_input_button_text_rect, (SDL_Color) {
                    255, 255, 255, 0
                });
                entry_text_input_button_text_rect.w /= 2;
                entry_text_input_button_text_rect.h /= 2;
            }
            SDL_RenderCopy(renderer, entry_text_input_button_text_texture, NULL, &entry_text_input_button_text_rect);
            break;
        }
        }

        // Up until now everything was drawn behind the scenes.
        // This will show the new, red contents of the window.
        SDL_RenderPresent(renderer);
    }


    SDL_DestroyTexture(title_texture);
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
