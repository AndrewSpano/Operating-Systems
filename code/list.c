#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"


NodePtr create_node(char* name, off_t offset, NodePtr prev, NodePtr next)
{
  NodePtr new_node = malloc(sizeof(node));
  if (new_node == NULL)
  {
    return NULL;
  }

  new_node->name = name;
  new_node->offset = offset;
  new_node->prev = prev;
  new_node->next = next;

  return new_node;
}


void node_destroy(NodePtr* node)
{
  free((*node)->name);
  free(*node);
  node = NULL;
}



Stack_List* create_List(void)
{
  Stack_List* new_List = malloc(sizeof(Stack_List));
  if (new_List == NULL)
  {
    perror("malloc");
    return NULL;
  }
  new_List->head = NULL;
  new_List->tail = NULL;
  new_List->size = 0;

  return new_List;
}




int is_Empty(Stack_List* list)
{
  return list->size == 0;
}



int is_in_Root(Stack_List* list)
{
  return list->size == 1;
}



int Stack_List_Push(Stack_List* list, char* name, off_t offset)
{
  if (is_Empty(list))
  {
    NodePtr new_node = create_node(name, offset, NULL, NULL);
    if (new_node == NULL)
    {
      return 0;
    }
    list->head = new_node;
    list->tail = new_node;
  }
  else
  {
    NodePtr new_node = create_node(name, offset, list->tail, NULL);
    if (new_node == NULL)
    {
      return 0;
    }
    list->tail->next = new_node;
    list->tail = new_node;
  }

  list->size++;

  return 1;
}



int Stack_List_Pop(Stack_List* list)
{
  if (is_Empty(list))
  {
    return 0;
  }

  if (list->size == 1)
  {
    node_destroy(&(list->tail));
    list->head = NULL;
    list->tail = NULL;
  }
  else
  {
    NodePtr temp = list->tail->prev;
    node_destroy(&(list->tail));
    list->tail = temp;
    temp->next = NULL;
  }


  list->size--;

  return 1;
}



int Stack_List_Peek(Stack_List* list, char** name, off_t* offset)
{
  if (is_Empty(list))
  {
    return 0;
  }

  strcpy(*name, list->tail->name);
  *offset = list->tail->offset;

  return 1;
}



int Stack_List_Print(Stack_List* list)
{
  if (is_Empty(list))
  {
    return 0;
  }

  NodePtr temp = list->head;
  while (temp != NULL)
  {
    printf("Directory: %s,\toffset: %lu\n", temp->name, temp->offset);
    temp = temp->next;
  }

  return 1;
}



int Stack_List_Print_Path(Stack_List* list)
{
  if (is_Empty(list))
  {
    return 0;
  }

  NodePtr temp = list->head;
  while (temp != NULL)
  {
    printf("/%s", temp->name);
    temp = temp->next;
  }
  printf("\n");

  return 1;
}



int Stack_List_Print_Directories(Stack_List* list, int n)
{
  if (list == NULL || n <= 0)
  {
    return 0;
  }

  uint number_of_prints = n;
  if (list->size < number_of_prints)
  {
    number_of_prints = list->size;
  }

  NodePtr temp = list->tail;
  int i = 1;
  for (; i < number_of_prints; i++)
  {
    temp = temp->prev;
  }


  // [0;34m	Blue
  // [1;34m	Bold Blue

  /* set print colour to Bold Blue */
  printf("\033[1;34m");

  printf("~");

  while (number_of_prints > 0)
  {
    printf("/%s", temp->name);
    temp = temp->next;
    number_of_prints--;
  }

  /* set print colour back to normal */
  printf("\033[0m");

  return 1;
}



int Stack_List_Empty(Stack_List* list)
{
  if (list == NULL || is_Empty(list))
  {
    return 0;
  }

  while (Stack_List_Pop(list));

  return 1;
}


int Stack_List_Destroy(Stack_List** list)
{
  if (*list == NULL)
  {
    return 0;
  }


  while (Stack_List_Pop(*list));



  (*list)->head = NULL;
  (*list)->tail = NULL;
  (*list)->size = 0;

  free(*list);
  *list = NULL;

  return 1;
}
