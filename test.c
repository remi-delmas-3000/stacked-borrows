#include "stacked_borrows.h"

typedef struct {
    int a;
    int b;
} my_struct_t;

int main() {
    SB_INIT(true, 8);

    my_struct_t my_struct;

    // for structs we initialise one stack per field
    NEW_LOCAL(my_struct.a);
    NEW_LOCAL(my_struct.b);

    int *toto = malloc(sizeof(int) * 10);

    // for arrays we initialise one stack for the whole array
    // if an individual cell gets borrowed we'll create a borrow stack lazily
    NEW_DYNAMIC(toto);

    // let a = &mut my_struct.a;
    USE1_LOCAL(my_struct.a);
    int *a = &my_struct.a;
    UNIQUE_FROM_LOCAL(a, my_struct.a);

    // let b = & my_struct.b;
    USE1_LOCAL(my_struct.b);
    int *b = &my_struct.b;
    UNIQUE_FROM_LOCAL(a, my_struct.a);
}