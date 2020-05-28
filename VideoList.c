#include "VideoList.h"

void insert(VideoList* list, Video* video) {
	Node* newNode = (Node*)malloc(sizeof(Node));
	newNode->video = video;
	if (!list->head) {
		list->head = newNode;
		return;
	}
	newNode->next = list->head;
	list->head = newNode;
}
