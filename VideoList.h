#include <stdlib.h>
#include "Video.h"

typedef struct Node {
	Video* video;
	struct Node* next;
} Node;

typedef struct VideoList {
	Node* head;
} VideoList;

void insert(VideoList* list, Video* video);
