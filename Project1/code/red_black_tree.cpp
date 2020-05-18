#include <iostream>
#include "red_black_tree.h"


RBT::RBT(void): root(NULL), size(0)
{ /* cout << "A Red Black Tree has been created!\n"; */ }

RBT::~RBT(void)
{ delete root; size = 0; /* cout << "A Red Black Tree is being destructed!\n"; */ }



/* Pretty much straightforward */
bool RBT::search(char* id)
{
  NodePtr temp = root;

  while (temp != NULL) {

    int order = temp->info->compare(id);                 /* variable that takes values < 0, = 0, > 0. It works like string::cmp(), returns < 0 if id < temp->info,
                                                            returns > 0 if id > temp->info and returns 0 if they are equal */
    if (order < 0) {
      temp = temp->left;
    } else if (order > 0) {
      temp = temp->right;
    } else {
      return true;
    }

  }

  return false;                          // we exited the while statement and no matching record was found, therefore it is not in the RB Tree
}



/* Pretty much straightforward */
voter* RBT::get_voter(char* id)
{
  NodePtr temp = root;

  while (temp != NULL) {

    int order = temp->info->compare(id);

    if (order < 0) {
      temp = temp->left;
    } else if (order > 0) {
      temp = temp->right;
    } else {
      return temp->info;
    }
  }

  return NULL;
}



/* Pretty much straightforward */
bool RBT::has_voted(char* id)
{
  NodePtr temp = root;

  while (temp != NULL) {

    int order = temp->info->compare(id);

    if (order < 0) {
      temp = temp->left;
    } else if (order > 0) {
      temp = temp->right;
    } else {
      return temp->hasvoted;
    }
  }

  return NULL;
}



/* Pretty much straightforward */
voter* RBT::vote(char* id)
{
  NodePtr temp = root;

  while (temp != NULL) {

    int order = temp->info->compare(id);

    if (order < 0) {
      temp = temp->left;
    } else if (order > 0) {
      temp = temp->right;
    } else {
      if (temp->hasvoted == true) {
        return NULL;
      } else {
        temp->hasvoted = true;
        return temp->info;
      }
    }
  }

  return NULL;
}




void RBT::left_rotation(NodePtr x)                   // Big thanks to the book Introduction to Algorithms
{
  NodePtr y = x->right;                              // x is the predecessor of y, something that will change when the left rotation is over
  x->right = y->left;                                // the right child of x becomes the left child of y

  if (y->left != NULL) {                             // in case that y has a left sub-child, we need to set its parent node to be x
    y->left->predecessor = x;
  }

  y->predecessor = x->predecessor;                   // the parent of y becomes the parent of x. The process to make y the parent of x while maintaining the properties of the RB Tree begins

  if (x->predecessor == NULL) {                      // if this statement is true, then the root of the tree was given as node x
    root = y;
  } else if (x == x->predecessor->left) {            // if this statement is true, then node x is the left sub-child of its parent node (x->predecessor)
    x->predecessor->left = y;
  } else {                                           // if we get here, it means that node x is the right sub-child of its parent node (x->predecessor)
    x->predecessor->right = y;
  }

  y->left = x;                                       // x becomes the left sub-child of y
  x->predecessor = y;                                // now y has become the parent of x
}




void RBT::right_rotation(NodePtr y)
{
  NodePtr x = y->left;                               // y is the predecessor of x, something that will change when the right rotation is over
  y->left = x->right;                                // the left child of y becomes the right child of x

  if (x->right != NULL) {                            // in case that x has a left sub-child, we need to se its parent node to y
    x->right->predecessor = y;
  }

  x->predecessor = y->predecessor;                   // the parent of x becomes the parent of y. The process to make x the parent of y while maintaining the properties of the RB Tree begins

  if (y->predecessor == NULL) {                      // if this statement is true, then the root of the tree was given as node y
    root = x;
  } else if (y == y->predecessor->right) {           // if this statement is true, then node y is the left sub-child of its parent node (y->predecessor)
    y->predecessor->right = x;
  } else {                                           // if we get here, it means that node y is the right sub-child of its parent node (y->predecessor)
    y->predecessor->left = x;
  }

  x->right = y;                                      // y becomes the left sub-child of x
  y->predecessor = x;                                // now x has become the parent of y
}




void RBT::insert(voter* item)
{
  NodePtr z = new node(RED, item, false, NULL, NULL, NULL);         // Create a new node, paint it RED, set its predecessor and children to NULL

  NodePtr y = NULL;
  NodePtr temp = root;

  while (temp != NULL) {                                             // Find the place where the new node will be placed. It will be in the very depth of the tree
    y = temp;
    if (item->compare(temp->info) < 0) {
      temp = temp->left;
    } else {
      temp = temp->right;
    }
  }

  z->predecessor = y;                                                // the node z will be placed under the node y
  size++;                                                            // a node has been added to the tree, increase the tree's size by 1

  if (y == NULL) {                                                   // if this condition is true, the tree was empty, therefore z will become the root and the process of insertion will be over
    root = z;
    z->colour = BLACK;                                               // make the colour of the node BLACK, so that the 2nd condition is satisfied (root must always be BLACK)
    return;
  } else if (z->info->compare(y->info) < 0) {                        // if this condition is true, the info of node z is smaller than that of node y, therefore z will become the left sub-child of y
    y->left = z;
  } else {                                                           // following the same logic as above, z will become the right sub-child of y
    y->right = z;
  }

  fix_insertion(z);
}




void RBT::fix_insertion(NodePtr z)
{
  NodePtr y = NULL;                                                             // y will be used to indicate z's uncle
  while ((z->predecessor != NULL) && z->predecessor->colour == RED) {           // while the colour of the parent of z is RED it means that one a property (2 or 4) of the RB Trees is being violated

    if (z->predecessor == z->predecessor->predecessor->left) {                  // if the parent of z is equal to the left sub-child of its parent
      y = z->predecessor->predecessor->right;                                   // y now becomes z's uncle

      /* CASE 1: Since we are inside the loop, the while condition is true. Therefore z's father (z->predecessor) is painted RED. Also, because we are inside the
                 if statement, z's father is the left sub-child of z's grandparent, which means that z has an uncle y (y can't be NULL because the condition 5
                 would not be satisfied). Case 1, which corresponds to the first part of the if statement below, happens when z's uncle (y) is also RED. Since
                 both nodes were red RED before z was inserted, by the condition 4, z's grandparent (z->predecessor->predecessor) must be BLACK. In order to fix
                 tree in this case, we need to paint BLACK both z's father and uncle, and PAINT RED z' grandparent. Intuitively, we can think that in order to
                 satisfy condition 5 (which states that every single path from the root to a leaf must have the same amount of BLACK nodes), we can "distribute"
                 the BLACK colour from z's grandparent to its children (z's father and uncle), because every path from the root to a leaf that contained z's
                 grandparent, also contained one of its children. Therefore the BLACK colour that z's grandparent had will be transfered to its children, so that
                 they help satisfying the 5th condition. But after that, we must paint RED z's grandparent so that there won't be extra BLACK nodes in the path. */
      if ((y != NULL) && (y->colour == RED)) {

        z->predecessor->colour = BLACK;
        y->colour = BLACK;
        z->predecessor->predecessor->colour = RED;
        /* we fixed the lowest 2 levels of the tree. Now z becomes its grandparent, and we keep iterating though the while loop in order to fix any other problems
           in the above levels that the addition of z might have caused to the RB Tree. All in all, we are working in a bottom - up way to restore the RB properties. */
        z = z->predecessor->predecessor;

      } else {
        /* CASE 2: z's uncle (y) is BLACK and z is the right sub-child of its parent (z->predecessor). In this case, both z and its predecessor are RED, causing the
                   4th property of the RB Trees to be violated (which states that a RED node must have 2 BLACK children). Case 2 is a "subset" of case 3. When the below
                   if statement is true, we need to make a left_rotation in order for it to "switch" roles with its parent. z will become the parent of its predecessor,
                   and z->predecessor will become z's left child. With this setup, we are ready to proceed to case 3. Note that if z was the left sub-child of its
                   predecessor, case 3 would come up automatically. We would not have needed to make the left rotation. Taking care of case 2 actually takes us to case 3. */
        if (z == z->predecessor->right) {
          z = z->predecessor;
          left_rotation(z);
        }
        /* CASE 3: z's uncle is BLACK and z is the left sub-child of its parent (z->predecessor). In this case, both z and its predecessor are RED, causing the
                   4th property of the RB Trees to be violated (which states that a RED node must have 2 BLACK children). In order to fix this problem, we have to
                   imagine the structure of the nodes. Let A = z, B = z->predecessor and C = z->predecessor->predecessor. The hierarchy is C -> B -> A. We need to
                   make B the middle node, place C to its right (because C->info > B->info) and A to its left (because B->info > A->info). This can be done using
                   a right rotation. After its over, since node A (which is z) is RED, its parent (which will be B) must be painted BLACK, otherwise the 4th property
                   of the RB Trees would be violated. Also, because Node c (z->predecessor->predecessor) was BLACK, we have to paint it RED in order to maintain the
                   5th property (every single path from the root to a leaf has the same amound of BLACK nodes). Again Intuitively, you can imagine that the colour
                   BLACK of node C gets "transfered" to B, which then gets "distributed" to A and C.  */
        z->predecessor->colour = BLACK;
        z->predecessor->predecessor->colour = RED;
        right_rotation(z->predecessor->predecessor);
      }

    } else {                                                                    // the parent of z is equal to the right sub-child of its parent

      /* The cases are exactly the same as above, only inverted left - right. Practically they are the same due to symmetry. Therefore they do not need any explanation. */

      y = z->predecessor->predecessor->left;                                    // y now becomes z's uncle

      /* CASE: 1 */
      if ((y != NULL) && y->colour == RED) {
        z->predecessor->colour = BLACK;
        y->colour = BLACK;
        z->predecessor->predecessor->colour = RED;
        z = z->predecessor->predecessor;

      } else {
        /* CASE: 2 */
        if (z == z->predecessor->left) {
          z = z->predecessor;
          right_rotation(z);
        }
        /* CASE: 3 */
        z->predecessor->colour = BLACK;
        z->predecessor->predecessor->colour = RED;
        left_rotation(z->predecessor->predecessor);
      }
    }

  }

  /* The colour of the root might have changed during the above operations. It has to be painted BLACK in order to satisfy condition 2 (root node must always be BLACK). */
  root->colour = BLACK;
}




/* Transplant node v in the place of node u. Node v CAN be NULL. Actually it will be NULL if we are trying to delete a node with 0 children.
   In order to take care of that we just have to add the if statement in the end of the transplant function. */
void RBT::transplant(NodePtr u, NodePtr v)
{
  if (u->predecessor == NULL) {
    root = v;
  } else if (u == u->predecessor->left) {
    u->predecessor->left = v;
  } else {
    u->predecessor->right = v;
  }
  if (v != NULL) {
    v->predecessor = u->predecessor;
  }
}




/* Returns the node with the minimum key from a Tree where it's root is the node given as a parameter (x) */
NodePtr RBT::min_of_subTree(NodePtr x)
{
  NodePtr temp = x;

  while (temp->left != NULL) {
    temp = temp->left;
  }

  return temp;
}




/* Removes a specific node from the RB Tree. If the removal of the node is successful, we return true. Else, we return false (when the node is not found) */
bool RBT::remove(char* id)
{
  NodePtr z = root;

  while (z != NULL) {                                      // Iterate through the RB Tree to find the node we are looking to delete

    short order = z->info->compare(id);                    /* variable that takes values < 0, = 0, > 0. It works like string::cmp(), returns < 0 if id < z->info,
                                                              returns 0 if id > z->info and returns > 0 if they are equal */
    if (order < 0) {
      z = z->left;
    } else if (order > 0) {
      z = z->right;
    } else {
      break;
    }
  }

  if (z == NULL) {                                         // if z is NULL, it means we iterated the whole Tree and did not find the voter we were searching for. Therefore return false.
    return false;
  }

  NodePtr x = NULL;
  /* temp_x is a temporary node that will be used instead of x in case that x is NULL. Because if x is NULL, we can't save its predecessor. Practically, temp_x will
     be a NULL node, which will serve as a disguise for the deletion algorithm. We need to know x's parent in order to call fix_deletion(x). We can't store the
     predecessor information if x is NULL. So in that case, we will store the same data to temp_x, and then call fix_deletion(temp_x). It will work, because even
     though temp_x has no actual information, it can store its predecessor node and its colour. */
  NodePtr temp_x = new node(BLACK, NULL, false, NULL, NULL, NULL);

  NodePtr y = z;
  bool initial_y_colour = y->colour;

  /* the if and else if cases are true when the node z we want to remove has less than 2 children. In that case we can just "replace" z with
     its child, if it has one, or with NULL, if it doesn't have one. */
  if (z->left == NULL) {

    if (z->right == NULL) {                                // if this condition is true, then temp_x will be used instead of x
      temp_x->predecessor = z;
      z->right = temp_x;
    } else {                                               // else, x is not NULL and therefore we can use it
      x = z->right;
    }

    transplant(z, z->right);

  } else if (z->right == NULL) {

    x = z->left;
    transplant(z, z->left);

  /* this else statement is triggered when the node we are trying to remove has 2 children. In this case, we must find smallest node y which is bigger than z.
     Then, we replace z with y. If y was a RED node, then we are ok. If y was a BLACK node, then we need to call fix_deletion() in order to fix the property
     5 of RB Tress that has been violated due to the removal of a BLACK node. */
  } else {

    y = min_of_subTree(z->right);
    initial_y_colour = y->colour;

    if (y->right == NULL) {
      temp_x->predecessor = y;
      y->right = temp_x;
    } else {
      x = y->right;
    }

    /* if x is not NULL and y is a child of z, make x->predecessor (== y->right->predecessor) equal to y, so that x will have y as a parent after z is deleted */
    if (y->predecessor == z) {

      if (x != NULL) {                                     // again we check if is NULL
        x->predecessor = y;
      } else {
        temp_x->predecessor = y;
      }

    /* else transplant x to y. Node y is "removed" from the Tree, but it saved. We will transplant it later to z.  */
    } else {
      transplant(y, y->right);
      y->right = z->right;
      y->right->predecessor = y;
    }

    /* transplanting y to z. Node y was removed from the tree for some steps, but now it comes back to replace node z. After this is over we can delete z. */
    transplant(z, y);
    y->left = z->left;
    y->left->predecessor = y;
    y->colour = z->colour;

  }

  z->left = NULL;                                          // set z's children to NULL, so that when we invoke the destructor of node,
  z->right = NULL;                                         // z's children won't get deleted
  delete z;                                                // Deleting the removed node

  if (initial_y_colour == BLACK) {                         // We replaced z with y. If y was RED then we are ok. If it was BLACK the some properties of the RB Tree may have been violated.

    /* depending on the value of x, we use the corresponding parameter for fix_deletion() */
    if (x != NULL) {
      fix_deletion(x);                                     // x is the first node that may have violated some of the properties of the RB Tree.
    } else {
      fix_deletion(temp_x);                                // temp_x is the first node that may have violated some of the properties of the RB Tree.
    }

  }

  /* In case temp_x was used, we need to make sure than no NodePtr points at it, because it's use was temporary and now its over. The pointers that pointed at
     temp_x can go back at just pointing to NULL. */
  if (temp_x->predecessor != NULL && temp_x == temp_x->predecessor->left) {
    temp_x->predecessor->left = NULL;
  } else if (temp_x->predecessor != NULL && temp_x == temp_x->predecessor->right) {
    temp_x->predecessor->right = NULL;
  }

  delete temp_x;                                           // delete the temporary node temp_x

  size--;                                                  // decrease the size of the RB Tree by one because a node was successfully removed
  if (size == 0) {
    /* if size reaches 0, it means that the Tree has been emptied, and therefore we must set the root to NULL so that it does not point to a deleted node. */
    root = NULL;
  }
  return true;                                             // return true because the deletion was successful
}





void RBT::fix_deletion(NodePtr x)
{
  while (x != root && x->colour == BLACK) {

    if (x == x->predecessor->left) {                                            // if x is a left child

      NodePtr w = x->predecessor->right;                                        // w is x's "sibling" node

      /* CASE 1: This case is a subset of the 3 other cases (2, 3 or 4), which occurs when w is a RED node. Therefore it has 2 BLACK children. In this situation we
                 must apply a left rotation to the parent of x. After the rotation, node w will have become the grandparent of x (w == x->predecessor->predecessor),
                 and w's right child will have become x's uncle. After that, we set w to be x's "sibling" again (w = x->predecessor->right). Going though this process,
                 helps us set up one of the 3 other cases. In other words, solving case 1 takes us to cases 2, 3 or 4. */
      if (w->colour == RED) {
        w->colour = BLACK;
        x->predecessor->colour = RED;
        left_rotation(x->predecessor);
        w = x->predecessor->right;
      }

      /* CASE 2: This case occurs when both w's children are BLACK. Due to property 4 of RB Trees (every RED node must have 2 BLACK children), we can paint w RED.
                 Since by removing a BLACK node we made some paths from the root to a leaf one BLACK node shorter, the 5th property has been violated. If we paint
                 w BLACK, we restore the 5h property, because all the paths than had lost a BLACK node, gain it back when w becomes BLACK. After that we set x to its
                 predecessor, and we keep iterating through the tree in order to fix any other problems that might have been caused. */
      if ((w->left == NULL || w->left->colour == BLACK) && (w->right == NULL || w->right->colour == BLACK)) {
        w->colour = RED;
        x = x->predecessor;
      } else {
        /* CASE 3: This case is a subset of case 4. This case occurs when w's right child is BLACK, and the left child is RED. In this occasion we can swap the
                   colours of w and its left child and then perform a right rotation without violating any of the properties of RB Trees. After this process is
                   over, x's "sibling" node will have become w->left. We set the new w to be w->left (or x->predecessor->right). Taking care of this case takes
                   us to case 4, which is presented below. */
        if (w->right == NULL || w->right->colour == BLACK) {
          w->left->colour = BLACK;
          w->colour = RED;
          right_rotation(w);
          w = x->predecessor->right;
        }
        /* CASE 4: This case occurs when w's right child is RED. By painting w the colour of its parent (x->predecessor == w->predecessor), then painting its
                   parent and right child BLACK, and then making a left rotation, we essentially manage to expunge the extra BLACK node that was violating
                   property 5. Therefore the Tree has now been re-established and we do not need to check for any other problems. We set x to be the root node
                   so that we will exit the while() statement in the next iteration. */
        w->colour = x->predecessor->colour;
        x->predecessor->colour = BLACK;
        w->right->colour = BLACK;
        left_rotation(x->predecessor);
        x = root;
      }

    /* The cases here are exactly the same with above, with the only difference that they are inverted left - right. There is no need to explain them again. */
    } else {                                                                    // else if x is a right child

      NodePtr w = x->predecessor->left;

      /* CASE: 1 */
      if (w->colour == RED) {
        w->colour = BLACK;
        x->predecessor->colour = RED;
        right_rotation(x->predecessor);
        w = x->predecessor->left;
      }

      /* CASE: 2 */
      if ((w->left == NULL || w->left->colour == BLACK) && (w->right == NULL || w->right->colour == BLACK)) {
        w->colour = RED;
        x = x->predecessor;
      } else {
        /* CASE: 3 */
        if (w->left == NULL || w->left->colour == BLACK) {
          w->right->colour = BLACK;
          w->colour = RED;
          left_rotation(w);
          w = x->predecessor->left;
        }
        /* CASE: 4 */
        w->colour = x->predecessor->colour;
        x->predecessor->colour = BLACK;
        w->left->colour = BLACK;
        right_rotation(x->predecessor);
        x = root;
      }
    }

  }

  /* The colour of the root may have changed during the above operations. So we paint it BLACK in order to satisfy property 2 of the RB Trees (the root node is always BLACK). */
  x->colour = BLACK;
}




void RBT::recreate_bloom_filter(BFPtr bf)
{
  if (size > 0) {
    Inorder_Recreation(bf, root);
  } else {
    cout << "Tree is empty, should have checked before calling RBT::recreate_bloom_filter().\n";
  }
}




void RBT::Inorder_Recreation(BFPtr bf, NodePtr temp)
{
  if (temp->left != NULL) {
    Inorder_Recreation(bf, temp->left);
  }

  bf->insert(temp->info->id);

  if (temp->right != NULL) {
    Inorder_Recreation(bf, temp->right);
  }
}




void RBT::print(FILE* output)
{
  recursive_print(root, output);
}




void RBT::recursive_print(NodePtr n, FILE* output)
{
  if (n->left != NULL) {
    recursive_print(n->left, output);
  }
  n->info->print(output);
  if (n->right != NULL) {
    recursive_print(n->right, output);
  }
}




/* A function that was used for testing the algorithms. It counts and prints the number of black nodes
   found from a single path from the root to any leaf of the RB Tree. The number should be equal for all leaves. */
void RBT::begin_count(void)
{
  if (root != NULL) {
    count_black_nodes(root, 1);
  } else {
    cout << "No size to count, the RB Tree is empty\n";
  }
}



/* A function that is used in the making of the above function. */
void RBT::count_black_nodes(NodePtr x, int count)
{
  if (x->colour == BLACK) {
    count++;
  }

  if (x->left != NULL) {
    count_black_nodes(x->left, count);
  } else {
    cout << "Number of black nodes to a left leaf:       " << count << endl;
  }

  if (x->right != NULL) {
    count_black_nodes(x->right, count);
  } else {
    cout << "Number of black nodes to a right leaf:      " << count << endl;
  }
}
