#include "../Node.h"


struct Game {
	Node *root_node;
};

 struct Node {
	 int number_of_flasks, flask_length;
	 int *list_of_flasks;

	 int number_of_moves;
	 int *list_of_moves;

	 struct Node *parent;
	 struct Node *child;

	 int *children;
	 int checked_children, children_length;
};

static void assign_available_moves(Node *node);

static void transfuse_flask(int *outgoing_flask,
                            int *incoming_flask,
                            Node *node,
                            const int *move,
                            unsigned int flask_length);

static void add_move(int **available_moves,
                     int giving_flask,
                     int receiving_flask,
                     int *moves_count,
                     int *available_moves_length);



//region memory mgmt functions

Node *create_node_root(char *configuration_path,
                       ssize_t string_length,
                       struct Game *game
) {
	Node *new_node = (Node *) malloc(sizeof(Node));
	new_node->list_of_moves = NULL;
	new_node->number_of_moves = 0;

	new_node->children = NULL;
	new_node->checked_children = 0;
	new_node->children_length = 0;

	new_node->parent = NULL;
	new_node->child = NULL;

	ssize_t char_read;
	int fd, index = 0;
	char buff[256], temp_buffer[20][100] = {};
	char delim[6] = " ,[]", line_delim[2] = "\n";

	if ((fd = open(configuration_path, 0)) == 0)
		perror("error on open\n");

	if ((char_read = read(fd, buff, string_length)) == -1)
		perror("error on reading: file too long\n");

	// useful to make strtok foolproof when there is no EOF in a file
	buff[char_read] = '\0';


	char *token = strtok(buff, line_delim);
	while (token != NULL && index < 20) {
		strcpy(temp_buffer[index++], token);
		token = strtok(NULL, line_delim);
	}


	int value;
	int *list_of_flasks = (int *) malloc(4 * index * sizeof(int));
	for (int i = 0; i < index; ++i) {
		token = strtok(temp_buffer[i], delim);
		for (int j = 0; j < 4; ++j) {
			if (token != NULL) {
				value = atoi(token);
				token = strtok(NULL, delim);
				list_of_flasks[4 * i + j] = value;
			} else
				list_of_flasks[4 * i + j] = 0;
		}
	}

	new_node->list_of_flasks = list_of_flasks;
	new_node->flask_length = 4;
	new_node->number_of_flasks = index;

	return new_node;
}

Node *create_node_children(Node *node,
                           int *move,
                           int flask_length,
                           int number_of_flasks
) {
	Node *new_node = (Node *) malloc(sizeof(Node));
	new_node->list_of_flasks = (int *) malloc(flask_length * number_of_flasks * sizeof(int));
	new_node->number_of_flasks = node->number_of_flasks;
	new_node->flask_length = node->flask_length;

	new_node->child = NULL;
	new_node->parent = node;

	int giving_flask[flask_length], receiving_flask[flask_length];
	transfuse_flask(giving_flask, receiving_flask, node, move, flask_length);

	for (int i = 0; i < number_of_flasks; ++i) {
		if (i == move[0]) {
			for (int j = 0; j < flask_length; ++j)
				new_node->list_of_flasks[4 * i + j] = giving_flask[j];
		}
		else if (i == move[1]) {
			for (int j = 0; j < flask_length; ++j)
				new_node->list_of_flasks[4 * i + j] = receiving_flask[j];
		}
		else {
			for (int j = 0; j < flask_length; ++j)
				new_node->list_of_flasks[4 * i + j] = node->list_of_flasks[4 * i + j];
		}
	}


	new_node->number_of_moves = node->number_of_moves + 1;
	new_node->list_of_moves = (int *) malloc(2 * new_node->number_of_moves * sizeof(int));
	for (int i = 0; i < node->number_of_moves; ++i) {
		new_node->list_of_moves[2 * i] = node->list_of_moves[2 * i];
		new_node->list_of_moves[2 * i + 1] = node->list_of_moves[2 * i + 1];
	}
	new_node->list_of_moves[2 * node->number_of_moves] = move[0];
	new_node->list_of_moves[2 * node->number_of_moves + 1] = move[1];

	new_node->children = NULL;
	new_node->checked_children = 0;
	new_node->children_length = 0;

	return new_node;
}

void delete_node_children(Node *node) {
	free(node->list_of_flasks);
	free(node->children);
	free(node->list_of_moves);
	free(node);
}

void delete_node_root(Node *node) {
	free(node->list_of_flasks);
	free(node->children);
	free(node->list_of_moves);
	free(node);
}

//endregion

//region control flow functions

int finished(Node *node) {
	for (int i = 0; i < node->number_of_flasks; ++i) {
		for (int j = node->flask_length - 2; j >= 0; --j) {
			if (node->list_of_flasks[4 * i + j] != node->list_of_flasks[4 * i + j + 1]) {
				return 0;
			}
		}
	}
	return 1;
}

int check_loop(Node *node) {
	int *configuration_list[4];
	Node *temp_node = node;
	for (int i = 0; i < 4; ++i) {
		configuration_list[i] = temp_node->list_of_flasks;
		if (temp_node->parent != NULL) {
			temp_node = temp_node->parent;
		}
		else return 0;
	}

	int test1_1 = 1, test1_2 = 1, test1_3 = 1;
	for (int i = 0; i < node->number_of_flasks; ++i) {
		for (int j = 0; j < node->flask_length; ++j) {
			test1_1 &= configuration_list[0][4 * i + j] == configuration_list[1][4 * i + j];
			test1_2 &= configuration_list[0][4 * i + j] == configuration_list[2][4 * i + j];
			test1_3 &= configuration_list[0][4 * i + j] == configuration_list[3][4 * i + j];
		}
	}
	int test1 = test1_1 || test1_2 || test1_3;


	int test2 = node->list_of_moves[2 * (node->number_of_moves - 1) + 1] == node->list_of_moves[2 * (node->number_of_moves - 3) + 1] &&
	            node->list_of_moves[2 * (node->number_of_moves - 1)] == node->list_of_moves[2 * (node->number_of_moves - 3)] &&
	            node->list_of_moves[2 * (node->number_of_moves - 2) + 1] == node->list_of_moves[2 * (node->number_of_moves - 4) + 1] &&
	            node->list_of_moves[2 * (node->number_of_moves - 2)] == node->list_of_moves[2 * (node->number_of_moves - 4)];

	return test1 || test2;
}

//endregion

//region Structure function


#define receiver_accepts_all(node, giving, receiving) (quantity_sent(node, giving) <= \
												((node)->flask_length - flask_size(node, receiving)))

static int move_is_valid(const Node* node, int* move) {
	if (!receiver_accepts_all(node, move[0], move[1])) {
		int a = 0;
		for (int i = 0; i < node->number_of_moves; ++i)
			if (move[0] == node->list_of_moves[2 * i]) a++;
		return a == 2;
	}
	return 1;
}


int assign_child_successful(Node *node) {
	if (node->children == NULL) {
		assign_available_moves(node);
	}

	if (node->checked_children >= node->children_length) return 0;

	while (!move_is_valid(node, &node->children[2 * node->checked_children])) {
		node->checked_children++;

		if (node->checked_children >= node->children_length) return 0;
	}

	int move[2] = {node->children[2 * node->checked_children],
	               node->children[2 * node->checked_children + 1]};
	node->child = create_node_children(node, move, node->flask_length, node->number_of_flasks);
	node->checked_children++;
	return 1;
}

#define flask_is_full(node, index) ((node)->list_of_flasks[4 * (index)] != 0)
#define flask_is_empty(node, index) ((node)->list_of_flasks[4 * (index) + (node)->flask_length - 1] == 0)
#define one_color_fill_flask(node, giving, receiving) (quantity_sent(node, giving) == \
												((node)->flask_length - flask_size(node, receiving)))
#define one_color_mount(node, giving, receiving) (flask_size(node, giving) == \
												((node)->flask_length - flask_size(node, receiving)))

static void assign_available_moves(Node *node) {

	int available_moves_length = 4, moves_count = 0;
	int *available_moves = (int *) malloc(2 * available_moves_length * sizeof(int));

	for (int giving_flask = 0; giving_flask < node->number_of_flasks; ++giving_flask) {
		for (int receiving_flask = 0; receiving_flask < node->number_of_flasks; ++receiving_flask) {

			if ((flask_is_full(node, receiving_flask)) ||
			    (flask_is_full(node, giving_flask) && has_one_color_only(node, giving_flask)) ||
			    (has_one_color_only(node, giving_flask) && flask_is_empty(node, receiving_flask)))
				continue;

			if (!flask_is_empty(node, giving_flask)) {
				if (!flask_is_empty(node, receiving_flask)) {
					// if (quantity_sent(node, giving_flask) > node->game->flask_length - flask_size(node, receiving_flask) &&)


					int going_flask_color = 0, coming_flask_color = 0;
					for (int k = 0; k < node->flask_length; ++k) {
						if (!going_flask_color) going_flask_color = node->list_of_flasks[4 * receiving_flask + k];
						if (!coming_flask_color) coming_flask_color = node->list_of_flasks[4 * giving_flask + k];
					}

					if (going_flask_color == coming_flask_color && giving_flask != receiving_flask) {
						add_move(&available_moves,
						         giving_flask,
						         receiving_flask,
						         &moves_count,
						         &available_moves_length);

						if ((has_one_color_only(node, giving_flask) &&
						     one_color_mount(node, giving_flask, receiving_flask)) ||
						    (has_one_color_only(node, receiving_flask) &&
						     one_color_fill_flask(node, giving_flask, receiving_flask))) {

							free(available_moves);

							int *shortcut_list = (int *) malloc(2 * sizeof(int));
							shortcut_list[0] = giving_flask, shortcut_list[1] = receiving_flask;
							node->children_length = 1;
							node->children = shortcut_list;
							return;
						}
					}
				} else {
					add_move(&available_moves,
					         giving_flask,
					         receiving_flask,
					         &moves_count,
					         &available_moves_length);
				}
			}
		}
	}
	node->children_length = moves_count;
	node->children = available_moves;
}

//endregion

//region utilities functions

int quantity_sent(const Node *node, int flask_number) {

	int color = 0, counter = 0;
	for (int i = 0; i < node->flask_length; ++i) {
		if (!color && node->list_of_flasks[4 * flask_number + i] != 0)
			color = node->list_of_flasks[4 * flask_number + i];

		if (color != 0 && node->list_of_flasks[4 * flask_number + i] == color) counter++;
		else break;
	}
	return counter;
}

int has_one_color_only(const Node *node, int flaskIndex) {
	int flask_length = node->flask_length;
	int *flask = &node->list_of_flasks[4 * flaskIndex];

	int retval = 1;
	if (flask_length < 2)
		perror("flask length too short\n");

	for (int i = flask_length - 2; i >= 0 ; --i) {
		if (flask[i] == 0)
			break;
		retval &= flask[flask_length - 1] == flask[i];
	}
	return retval;
}

int flask_size(const Node* node, int flask_number) {
	int i;
	for (i = 0; i < node->flask_length; ++i)
		if (node->list_of_flasks[4 * flask_number + i] != 0) break;

	return node->flask_length - i;
}

//endregion

//region static functions

static void transfuse_flask(int *outgoing_flask,
                            int *incoming_flask,
                            Node *node,
                            const int *move,
                            unsigned int flask_length) {
	for (int i = 0; i < flask_length; ++i) {
		outgoing_flask[i] = node->list_of_flasks[4 * move[0] + i];
		incoming_flask[i] = node->list_of_flasks[4 * move[1] + i];
	}

	int available_space;
	int transferable_fluid, start = 0, end = 0;
	for (available_space = 0;
	     incoming_flask[available_space] == 0 && available_space < flask_length; available_space++) {}
	while (outgoing_flask[start] == outgoing_flask[end]) {
		if (outgoing_flask[start] == 0) start++;
		end++;
	}
	transferable_fluid = end - start;

	int temp;
	for (int i = 0; i < transferable_fluid && i < available_space; ++i) {
		temp = incoming_flask[available_space - i - 1];
		incoming_flask[available_space - i - 1] = outgoing_flask[start + i];
		outgoing_flask[start + i] = temp;
	}
}

static void add_move(int **available_moves,
                     int giving_flask,
                     int receiving_flask,
                     int *moves_count,
                     int *available_moves_length) {

	if (*moves_count >= *available_moves_length) {
		*available_moves_length *= 2;
		int *temp = *available_moves;
		*available_moves = (int *) malloc(2 * *available_moves_length * sizeof(int));
		for (int k = 0; k < *moves_count; ++k) {
			(*available_moves)[2 * k] = temp[2 * k];
			(*available_moves)[2 * k + 1] = temp[2 * k + 1];
		}
		free(temp);
	}
	(*available_moves)[2 * (*moves_count)] = giving_flask;
	(*available_moves)[2 * (*moves_count) + 1] = receiving_flask;
	(*moves_count)++;
}

//endregion