#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <jansson.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>

#define PORT 7777
#define MAX 2000
#define SA struct sockaddr
#define MAX_CLIENTS 4
#define NUM_MATERIALS 5
#define MATERIAL_SIZES 3

void exit_handler();

char *receive(const int *client);

void send_to(const int *client, char *json_string);

void *handle_connection(void *);

double getWeight(const json_t *root);

void requests(int *);

char *get_city_name(int);

char *create_material(int);

char *generate_two_random_numbers();

int BeachedThings();

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barrier;
pthread_t threads[MAX_CLIENTS] = {0};
int clients_sockets_array[MAX_CLIENTS] = {0};
char *buffer_cargos_from[MAX_CLIENTS] = {NULL};
int listen_sock;

enum id {
    PortKC, LakeKC, SouthKC, MountainKC,
};

typedef struct {
    int client_socket;
    int client_id;
    char *city_name_string;
} thread_client_info;


int main() {
    signal(SIGINT, exit_handler);

    struct sockaddr_in servaddr, cli;
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        printf("Socket not created...\n");
        exit(0);
    }

    memset(&servaddr, '\0', sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    int optval = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(listen_sock, (SA *) (&servaddr), sizeof(servaddr)) != 0) {
        perror("Bind not successfully\n");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_sock, 7) != 0) {
        perror("Listening not successfully\n");
        exit(EXIT_FAILURE);
    }
    socklen_t client_len = sizeof(cli);
    pthread_barrier_init(&barrier, NULL, MAX_CLIENTS);
    printf("*****CHIRAL NETWORK HAS STARTED ITS WORK*****\n\n");
    printf("*****Capital Knot City has entered to Chiral Network,central port is established.\n");

    for (int num_clients = 0; num_clients < MAX_CLIENTS; ++num_clients) {
        int client_socket = accept(listen_sock, (SA *) &cli, &client_len);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }
        thread_client_info *city_info = malloc(sizeof(thread_client_info));
        if (city_info == NULL) {
            perror("Failed to allocate memory");
            exit(EXIT_FAILURE);
        }
        city_info->client_socket = client_socket;
        city_info->client_id = num_clients;
        city_info->city_name_string = get_city_name(num_clients);

        if (pthread_create(&threads[num_clients], NULL, handle_connection, (void *) city_info) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            continue;
        }
        clients_sockets_array[num_clients] = client_socket;
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Thread join failed");
            exit(EXIT_FAILURE);
        }
    }

    printf("\n\n*****CHIRAL NETWORK HAS ENDED ITS WORK*****!\n\n");

    exit_handler();
    return 0;
}

void *handle_connection(void *arg) {
    signal(SIGINT, exit_handler);
    thread_client_info city_info = *(thread_client_info *) arg;
    free(arg);

    char *greetings = malloc(MAX);
    sprintf(greetings, "%d", city_info.client_id);
    send_to(&city_info.client_socket, greetings);
    free(greetings);

    pthread_mutex_lock(&mutex);
    printf("\n*****%s has entered to Chiral Network.\n", city_info.city_name_string);
    pthread_mutex_unlock(&mutex);
    pthread_barrier_wait(&barrier);

    switch (city_info.client_id) {
        case PortKC:
            printf("\n*****CHIRAL NETWORK HAS STARTED DELIVERING*****\n\n");
            sleep(1);

            while (1) {
                buffer_cargos_from[PortKC] = create_material(100);
                if (buffer_cargos_from[PortKC] == NULL) {
                    continue;
                }
                if (BeachedThings() == 0) {
                    break;
                }
                free(buffer_cargos_from[PortKC]);
            }
            printf("\n*****Port Knot City is waiting for us!!!****\n");
            break;
        default:
            pthread_mutex_lock(&cond_mutex);
            while (buffer_cargos_from[city_info.client_id] == NULL) {
                pthread_cond_wait(&cond, &cond_mutex);
            }
            pthread_mutex_unlock(&cond_mutex);
            break;
    }

    send_to(&city_info.client_socket, buffer_cargos_from[city_info.client_id]);

    if (city_info.client_id != MountainKC) {
        requests(&city_info.client_socket);

        char *cargo_from = receive(&city_info.client_socket);
        if (cargo_from == NULL)
            exit_handler();

        pthread_mutex_lock(&mutex);
        buffer_cargos_from[city_info.client_id + 1] = cargo_from;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

char *receive(const int *client) {
    char *json_string = malloc(MAX);
    if (json_string == NULL) {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    memset(json_string, 0, MAX);
    ssize_t numRead = recv(*client, json_string, MAX, 0);
    if (numRead <= 0) {
        free(json_string);
        return NULL;
    }

    json_string[numRead] = '\0';
    return json_string;
}

void send_to(const int *client, char *json_string) {
    size_t length = strlen(json_string);
    if (send(*client, json_string, length, 0) != length) {
        perror("not all written");
        free(json_string);
        exit(EXIT_FAILURE);
    }
}

char *create_material(int stamina) {
    json_t *root = json_object();
    json_t *carried_materials = json_object();
    json_object_set_new(root, "destination", json_string("Port Knot City"));
    json_object_set_new(root, "name", json_string("Sam"));
    json_object_set_new(root, "surname", json_string("Porter Bridges"));
    json_object_set_new(root, "carried_materials", carried_materials);
    json_t *resins = json_object();
    json_t *metals = json_object();
    json_t *ceramics = json_object();
    json_t *chemicals = json_object();

    printf("\n*Enter a materials.");
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
        printf("\n*Too heavy,try again\n");
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
        if (i == 4 && material != NULL) {
            weight += (double) json_integer_value(material) * crystals;
            continue;
        }
        if (!material || !json_is_object(material)) {
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
        }
        json_decref(material);
    }

    json_decref(carried_materials);
    return weight;
}

void requests(int *sock) {
    while (1) {
        char *request = receive(sock);
        if (request == NULL) {
            perror("Error receiving");
            exit_handler();
        }
        if (strcmp(request, "/recover") == 0) {
            send_to(sock, "confirmed");
            free(request);
        } else if (strcmp(request, "/roll") == 0) {
            send_to(sock, generate_two_random_numbers());
            free(request);
        } else {
            free(request);
            break;
        }
    }
}

char *generate_two_random_numbers() {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1) {
        perror("Error open");
        exit(EXIT_FAILURE);
    }

    unsigned char random_bytes[2];
    if (read(fd, random_bytes, sizeof(random_bytes)) != sizeof(random_bytes)) {
        perror("Error reading");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    if (((random_bytes[0] % 6) + 1 + (random_bytes[1] % 6) + 1) >= 6) {
        return "confirmed";
    } else
        return "rejected";
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
            if (strcmp(generate_two_random_numbers(), "confirmed") == 0) {
                printf("\nYou have won.\n");
                return 0;
            } else {
                printf("\n*****Cargo is stolen,returning back...\n");
                return 1;
            }
        } else
            printf("\nInvalid.Please enter correctly.\n");
    }
}

void exit_handler() {
    printf("\nExiting...\n");
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (threads[i] != 0)
            pthread_kill(threads[i], SIGKILL);
        close(clients_sockets_array[i]);
        free(buffer_cargos_from[i]);
    }
    close(listen_sock);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&cond_mutex);
    pthread_cond_destroy(&cond);
    exit(EXIT_FAILURE);
}
