#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <jansson.h>
#include <signal.h>

#define PORT 7777
#define MAX 999
#define SA struct sockaddr
#define NUM_MATERIALS 5
#define MATERIAL_SIZES 3
#define MAX_CLIENTS 4

enum id {
    PortKC, LakeKC, SouthKC, MountainKC,
};

int recover();

int BeachedThings();

void exit_handler();

char *create_material(int, int);

void send_to_server(char *json_string);

char *receive_from_server();

double getWeight(const json_t *);

long long getStamina(char *);

char *get_city_name(int id);

int get_city_id();

int sock;

int main() {
    signal(SIGINT, exit_handler);

    struct sockaddr_in cli;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket not created");
        exit_handler();
    }

    memset(&cli, 0, sizeof(cli));
    cli.sin_family = AF_INET;
    cli.sin_addr.s_addr = inet_addr("127.0.0.1");
    cli.sin_port = htons(PORT);

    if (connect(sock, (SA *) (&cli), sizeof(cli)) != 0) {
        perror("Connection failed");
        close(sock);
        exit_handler();
    }

    int city_id = get_city_id();

    printf("*****Welcome to Chiral Network, %s.\n", get_city_name(city_id));
    sleep(1);
    printf("\nOur cargo is on the way, wait...\n");


    char *received_cargo = receive_from_server();
    if (received_cargo == NULL) {
        close(sock);
        exit_handler();
    }

    if (city_id == MountainKC) {
        printf("\n*Cargo is delivered:\n%s\n\n", received_cargo);
        free(received_cargo);
        exit_handler();
        return 0;
    }

    printf("\n*Cargo is delivered:\n%s\n\nGetting ready to next delivery...\n", received_cargo);
    fflush(stdout);
    sleep(1);

    int stamina = (int) getStamina(received_cargo);
    if (stamina == 0) {
        printf("\nWe are out of stamina,type /recover for a rest.\n");
        stamina = recover();
    }
    printf("\nNow,we have %d stamina\n", stamina);

    char *cargo_to;
    while (1) {
        cargo_to = create_material(city_id + 1, stamina);
        if (cargo_to == NULL) {
            continue;
        }
        if (BeachedThings() == 0)
            break;
        free(cargo_to);
    }

    printf("\n*****%s is waiting for us!!!****\n", get_city_name(city_id + 1));
    sleep(1);
    send_to_server(cargo_to);
    free(received_cargo);
    free(cargo_to);

    return 0;
}

char *receive_from_server() {
    char *json_string = malloc(MAX);
    if (json_string == NULL) {
        perror("Error allocating memory");
        exit_handler();
    }
    memset(json_string, 0, MAX);
    ssize_t numRead = recv(sock, json_string, MAX, 0);
    switch (numRead) {
        case 0:
            printf("Server closed connection\n");
            free(json_string);
            exit_handler();
        case -1:
            perror("Error reading from socket");
            free(json_string);
            exit_handler();
        default:
            return json_string;
    }
}

char *create_material(int id, int stamina) {
    json_t *root = json_object();
    json_t *carried_materials = json_object();
    json_object_set_new(root, "destination", json_string(get_city_name(id)));
    json_object_set_new(root, "name", json_string("Sam"));
    json_object_set_new(root, "surname", json_string("Porter Bridges"));
    json_object_set_new(root, "carried_materials", carried_materials);
    json_t *resins = json_object();
    json_t *metals = json_object();
    json_t *ceramics = json_object();
    json_t *chemicals = json_object();

    printf("\n*Enter a materials to send.\n");
    const char *sizes_of_material[MATERIAL_SIZES] = {"small", "medium", "large"};
    const char *material_names[NUM_MATERIALS] = {"resins", "metals", "ceramics", "chemicals", "crystals"};

    for (int i = 0; i < NUM_MATERIALS; ++i) {
        for (int j = 0; j < MATERIAL_SIZES; ++j) {
            printf("\n");
            if (i == 4) {
                printf("Enter the number of crystals: ");
                j = MATERIAL_SIZES;
            } else
                printf("Enter the number of %s %s:", sizes_of_material[j], material_names[i]);

            char input[MAX];
            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("Error reading input.\n");
                exit_handler();
            }
            char *endptr;
            long result = strtol(input, &endptr, 10);

            if ((*endptr != '\0' && *endptr != '\n')) {
                printf("Invalid input. Please enter an integer.\n");
                --j;
                while (getchar() != '\n');
                continue;
            }
            if (result != 0) {
                switch (i) {
                    case 0:
                        json_object_set_new(resins, sizes_of_material[j], json_integer(result));
                        break;
                    case 1:
                        json_object_set_new(metals, sizes_of_material[j], json_integer(result));
                        break;
                    case 2:
                        json_object_set_new(ceramics, sizes_of_material[j], json_integer(result));
                        break;
                    case 3:
                        json_object_set_new(chemicals, sizes_of_material[j], json_integer(result));
                        break;
                    case 4:
                        json_object_set_new(carried_materials, material_names[4], json_integer(result));
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (json_object_size(resins) != 0) json_object_set_new(carried_materials, material_names[0], resins);

    if (json_object_size(metals) != 0) json_object_set_new(carried_materials, material_names[1], metals);

    if (json_object_size(ceramics) != 0) json_object_set_new(carried_materials, material_names[2], ceramics);

    if (json_object_size(chemicals) != 0) json_object_set_new(carried_materials, material_names[3], chemicals);

    json_t *copy = json_deep_copy(root);
    double calculated_weight = getWeight(copy);
    json_decref(copy);
    if (calculated_weight > 149) {
        printf("\nToo heavy,try again\n");
        json_decref(root);
        json_decref(carried_materials);
        json_decref(resins);
        json_decref(metals);
        json_decref(ceramics);
        json_decref(chemicals);
        return NULL;
    }
    stamina = stamina - ((int) calculated_weight / 2);
    char formatted_value[MAX];
    snprintf(formatted_value, sizeof(formatted_value), "%.3f", calculated_weight);

    json_object_set_new(root, "load_weight", json_string(formatted_value));
    json_object_set_new(root, "stamina", json_integer(stamina));
    char *json_string = json_dumps(root, JSON_INDENT(4));
    json_decref(root);
    json_decref(carried_materials);
    json_decref(resins);
    json_decref(metals);
    json_decref(ceramics);
    json_decref(chemicals);
    if (json_string == NULL) {
        perror("Error dumps");
        exit_handler();
    }
    return json_string;
}

void send_to_server(char *json_string) {
    size_t length = strlen(json_string);
    if (send(sock, json_string, length, 0) != length) {
        perror("not all written");
        free(json_string);
        exit_handler();
    }
}

long long getStamina(char *root) {
    json_t *cargo = json_loads(root, 0, NULL);
    if (!cargo || !json_is_object(cargo)) {
        perror("getStamina");
        exit_handler();
    }
    json_t *stamina = json_object_get(cargo, "stamina");
    int stamina_value = (int) json_integer_value(stamina);
    json_decref(cargo);
    json_decref(stamina);
    if (stamina_value > 50) {
        return stamina_value;
    } else {
        return 0;
    }
}

double getWeight(const json_t *root) {
    double weight = 0;
    int resins_weight[MATERIAL_SIZES] = {4, 6, 8};
    int metals_weight[MATERIAL_SIZES] = {5, 8, 10};
    int ceramics_weight[MATERIAL_SIZES] = {4, 6, 8};
    int chemicals_weight[MATERIAL_SIZES] = {3, 4, 5};
    double crystals = 0.1f;

    json_t *carried_materials = json_object_get(root, "carried_materials");
    if (!carried_materials || !json_is_object(carried_materials)) {
        return weight;
    }

    char *sizes[MATERIAL_SIZES] = {"small", "medium", "large"};
    const char *material_names[NUM_MATERIALS] = {"resins", "metals", "ceramics", "chemicals", "crystals"};
    for (int i = 0; i < NUM_MATERIALS; ++i) {
        json_t *material = json_object_get(carried_materials, material_names[i]);
        if (!material || !json_is_object(material)) {
            continue;
        }
        if (i == 4) {
            long long count = json_integer_value(material);
            weight += (double) count * crystals;
            continue;
        }
        for (int j = 0; j < MATERIAL_SIZES; ++j) {
            json_t *size_count = json_object_get(material, sizes[j]);
            if (size_count) {
                long long count = json_integer_value(size_count);
                int *weight_array = NULL;
                switch (i) {
                    case 0:
                        weight_array = resins_weight;
                        break;
                    case 1:
                        weight_array = metals_weight;
                        break;
                    case 2:
                        weight_array = ceramics_weight;
                        break;
                    case 3:
                        weight_array = chemicals_weight;
                        break;
                    default:
                        break;
                }
                if (weight_array) {
                    weight += (double) (count * weight_array[j]);
                }
            }
            json_decref(size_count);
        }
        json_decref(material);
    }
    json_decref(carried_materials);
    return weight;
}

int BeachedThings() {
    printf("\nYou have been attacked by the Beached Things!Type '/roll' for battling!\n");
    char input[MAX];
    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            exit_handler();
        }
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "/roll") == 0) {
            send_to_server("/roll");
            char *recv_buffer = receive_from_server();
            if (strcmp(recv_buffer, "confirmed") == 0) {
                printf("\nYou have won.\n");
                free(recv_buffer);
                send_to_server("go");
                return 0;
            } else {
                printf("\n******Cargo is stolen,returning back...\n");
                free(recv_buffer);
                return 1;
            }
        } else
            printf("\nInvalid.Please enter correctly.\n");
    }
}

int recover() {
    char input[MAX];
    while (1) {
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            exit_handler();
        }
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "/recover") == 0) {
            send_to_server("/recover");
            char *recv_buffer = receive_from_server();
            if (strcmp(recv_buffer, "confirmed") == 0) {
                printf("\nYou have been recovered!\n");
                free(recv_buffer);
                break;
            } else {
                printf("\nYou have not been recovered!\n");
                free(recv_buffer);
                exit_handler();
            }
        } else
            printf("\nInvalid.Please enter /recover.\n");
    }
    return 100;
}

char *get_city_name(int id) {
    switch (id) {
        case PortKC:
            return "Port Knot City";
        case LakeKC:
            return "Lake Knot City";
        case SouthKC:
            return "South Knot City";
        case MountainKC:
            return "Mountain Knot City";
        default:
            exit_handler();
            return NULL;
    }
}

int get_city_id() {
    char *str = receive_from_server();
    if (str == NULL) {
        close(sock);
        exit_handler();
    } else
        str[strcspn(str, "\n")] = 0;
    char *endptr;
    int city_id = (int) strtol(str, &endptr, 10);
    if (*endptr != '\0') {
        perror("strtol");
        free(str);
        exit_handler();
    }
    free(str);
    return city_id;
}

void exit_handler() {
    printf("\nExiting...\n");
    close(sock);
    exit(EXIT_SUCCESS);
}
