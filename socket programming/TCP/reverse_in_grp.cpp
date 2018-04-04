#include <iostream>
using namespace std;

struct node
{
    int val;
    node *next;
};

void reverse_in_grp(node **headref, int size);
void reverse(node **front, node **back);
void push(node **headref, int a);

void reverse_in_grp(node **headref, int size)
{
    int i;
    node *last_node = *headref;
    if( *headref == NULL)
    {
        return;
    }
    
    
    for( i=0; i < size-1; i++)
    {
        if(last_node ->next == NULL)
        break;
        last_node = last_node -> next;
    }
    
    reverse(headref, &last_node);
    
    if(last_node ->next != NULL)
    {
        reverse_in_grp(&(last_node -> next), size);
    }
    
    return;
}

void reverse(node **front, node **back)
{
    if( *front == *back) return;
    node *next_list = (*front)->next;
    if( next_list != NULL )
    reverse(&next_list, back);
    (*back)->next = *front;
    *front = next_list;
    *back = *front;
    return;
}

void push(node **headref, int a)
{
    node *tmp = new node;
    tmp -> next = *headref ;
    tmp -> val = a;
    *headref = tmp ;
    return;
}

void print(node *x)
{
    if(x == NULL) return;
    cout << x -> val;
    print(x->next) ;
    return;
}
int main() {
	node *head = NULL ;
	push ( &head, 1);
	push ( &head, 2);
	push ( &head, 3);
	push ( &head, 4);
	print(head);
	cout <<"a";
	reverse(&head, &(head->next)) ;
	return 0;
}
