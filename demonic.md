demonic-non determinism is just a concept that lets you trade space for nondeterminism when refuting universal properties. If you want to prove that P(i) holds for all i, instead of really asserting P(i) for all i, you introduce an existentially quantified i (which makes it nondet) and just try to satisfy \lnot P(i) if you cannot satisfy \lnot P(i) for any nondet i then P(i) holds for all i.   All you need to use the concept is to know what class of property you are after, and access to nondet variables, which are readily available in Kani/CBMC.





Let’s say you have a program that manipulates an array of integers, and at some point you want to prove a[i] == 0 for all i in range of the array:

```C
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// this is how nondeterministic functions are declared
size_t __VERIFIER_nondet_size_t();

#define A_SIZE 5
int main()
{
	int a[A_SIZE]; // uninitialised
	__CPROVER_havoc_object(a);

	// the code does something to each cell of the the array
   // (because it's a real program that has to do actual stuff to the memory in the real world)
	for (size_t i = 0; i < A_SIZE; i++)
	{
		a[i] = 0;
	}

  // using demonic nondeterminism to prove forall i, a[i] == 0
	size_t demonic_i = __VERIFIER_nondet_size_t();
	if (demonic_i < A_SIZE) {
		assert(a[demonic_i] == 0);
	}
	return 0;
}
```

In that model, `demonic_i` is existentially quantified, and the assertion asks the question: “is there a value for `demonic_i` that can falsify `a[demonic_i]==0` ?”. If there is a value `K` allowed by the program where such that `a[K]==0` CBMC will pick `demonic_i == K` and produce a counter example.

If I run this through the tool I get:
```bash
** Results:
toto.c function main
[main.assertion.1] line 23 assertion a[demonic_i] == 0: SUCCESS

** 0 of 1 failed (1 iterations)
VERIFICATION SUCCESSFUL
```


If I change the program to skip initializing some cell:
```C
	// the code does something to the array
	for (size_t i = 0; i < A_SIZE; i++)
	{
		if (i==3) // skipping initialisation
			continue;
		a[i] = 0;
	}
```


I get the following trace:

```bash
State 48 file toto.c function main line 23 thread 0
----------------------------------------------------
  demonic_i=3ul (00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000011)

Violated property:
  file toto.c function main line 25 thread 0
  assertion a[demonic_i] == 0
  !((signed long int)(signed long int)!(a[(signed long int)demonic_i] == 0) != 0l)

** 1 of 1 failed (2 iterations)
VERIFICATION FAILED
```

Our nondet oracle found the skipped cell by picking the value `3` for `demonic_i`. 
It even works if the program makes nondeterministic decisions itself:

```C
	// pick a cell to skip;
	int skipped =  __VERIFIER_nondet_size_t();

	// the code does something to the array
	for (size_t i = 0; i < A_SIZE; i++)
	{
		if (i==skipped)
			continue;
		a[i] = 0;
	}
```

```bash
State 21 file toto.c function main line 15 thread 0
----------------------------------------------------
  skipped=4 (00000000 00000000 00000000 00000100)
...

State 52 file toto.c function main line 25 thread 0
----------------------------------------------------
  demonic_i=4ul (00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000100)

Violated property:
  file toto.c function main line 27 thread 0
  assertion a[demonic_i] == 0
  !((signed long int)(signed long int)!(a[(signed long int)demonic_i] == 0) != 0l)

** 1 of 1 failed (2 iterations)
VERIFICATION FAILED
```

The first oracle picked cell 4 to skip, and the second oracle found that cell 4 was skipped.

Now let’s say I have two arrays for which I want to check `forall i, A[i] == 0`.
I can pick which one to check nondeterministically, and pick which cell to check nondeterministically within that array:

```C
int main()
{
	int a[A_SIZE]; // uninitialised
	__CPROVER_havoc_object(a);

	int b[A_SIZE]; // uninitialised
	__CPROVER_havoc_object(b);

	// pick a cell to skip;
	int skipped =  __VERIFIER_nondet_size_t();

	// the code does something to the array
	for (size_t i = 0; i < A_SIZE; i++)
	{
		a[i] = 0;
	}

	// the code does something to the array
	for (size_t i = 0; i < A_SIZE; i++)
	{
		b[i] = 0;
	}

	int *demonic_array = __VERIFIER_nondet_size_t() ? a: b;
	size_t demonic_i = __VERIFIER_nondet_size_t();
	if (demonic_i < A_SIZE) {
		assert(demonic_array[demonic_i] == 0);
	}
	return 0;
}
```

And for the last step, I don’t have to wait until the end wether to pick a, b, or which index to pick within a or b, I can make these demonic choices along with execution:

```C
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// this is how nondeterministic functions are declared
size_t __VERIFIER_nondet_size_t();

int *demonic_array = NULL;
size_t demonic_i = 0;

#define A_SIZE 5
#define B_SIZE 10
int main()
{
	int a[A_SIZE];
	__CPROVER_havoc_object(a);
	if (__VERIFIER_nondet_size_t()) // <- demonic choice
	{
		// pick a cell in a to monitor
		demonic_array = a;
		demonic_i = __VERIFIER_nondet_size_t(); // <- demonic choice
		__CPROVER_assume(demonic_i < A_SIZE);
	}

	int b[B_SIZE];
	__CPROVER_havoc_object(b);
	if (__VERIFIER_nondet_size_t()) // <- demonic choice
	{
		// pick a cell in b to monitor
		demonic_array = b;
		demonic_i = __VERIFIER_nondet_size_t(); // <- demonic choice
		__CPROVER_assume(demonic_i < B_SIZE);
	}
	// the code does something to the array
	for (size_t i = 0; i < A_SIZE; i++)
	{
		a[i] = 0;
	}

	// the code does something to the array
	for (size_t i = 0; i < B_SIZE; i++)
	{
		b[i] = 0;
	}

	if (demonic_array != NULL) {
		// the oracle picked a cell to monitor in either a or b,
        // I can check the property I want
		assert(demonic_array[demonic_i] == 0);
	}

	return 0;
}
```

With that formulation, if we inject a bug in either a or b’s initialisation, we will ultimately find it. The nondet choice of which location to ultimately check is made ahead of time but it does not matter.

So this is how we make nondet choices that are demonic, i.e. guaranteed to reveal violations of universal properties:

```C
	int fresh[SIZE]; // <- fresh object 
	if (__VERIFIER_nondet_size_t()) // <- demonic choice : pick the fresh object or not
	{
		demonic_array = fresh;
		demonic_i = __VERIFIER_nondet_size_t(); // <- demonic choice : pick an index to monitor 
		__CPROVER_assume(demonic_i < SIZE); // assume the index is in range of the array we picked
	}
```

Once we have done this, we have either picked a cell to monitor in the fresh array, or we just keep monitoring whatever we were before.

Now I extend the model to count how many times array cells get updated, by adding a count variable to the ghost state

```C
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// this is how nondeterministic functions are declared
size_t __VERIFIER_nondet_size_t();

// the array we decide to monitor (ghost state)
int *demonic_array = NULL;

// the index in the array we decide to monitor (ghost state)
size_t demonic_i = 0;

// the count of number of writes for that monitored array cell (ghost state)
size_t count = 0;

#define A_SIZE 5
#define B_SIZE 10
int main()
{
	int a[A_SIZE];
	__CPROVER_havoc_object(a);

    // ghost code
	if (__VERIFIER_nondet_size_t())
	{
		// pick a cell in a to monitor
		demonic_array = a;
		demonic_i = __VERIFIER_nondet_size_t();
		__CPROVER_assume(demonic_i < A_SIZE);
		count = 0;
	}

	int b[B_SIZE];
	__CPROVER_havoc_object(b);

    // ghost code
	if (__VERIFIER_nondet_size_t())
	{
		// pick a cell in a to monitor
		demonic_array = b;
		demonic_i = __VERIFIER_nondet_size_t();
		__CPROVER_assume(demonic_i < B_SIZE);
		count = 0;
	}

	// the code does something to the array
	for (size_t i = 0; i < A_SIZE; i++)
	{
		a[i] = 0;
         // ghost code
		if (__CPROVER_same_object(demonic_array, a) && demonic_i == i) {
		    // update shadow count for a[i] if it is currently tracked
			count += 1;
		}
	}

	// the code does something to the array
	for (size_t i = 0; i < B_SIZE; i++)
	{
		b[i] = 0;
         // ghost code
		if (__CPROVER_same_object(demonic_array, b) && demonic_i == i) {
	    	// update shadow count for b[i] if it is currently tracked
			count += 1;
		}
	}

	if (demonic_array != NULL) {
		// now I can check the property I want
		assert(demonic_array[demonic_i] == 0);
		// cell was only written once
		assert(count == 1);
	}

	return 0;
}
```

I can prove that any cell in a or b is only assigned once.

If I modify the program to assign `a[2]` twice, and Instrument that second assignment with ghost code too:

```C
	// the code does something to the array
	for (size_t i = 0; i < A_SIZE; i++)
	{
		a[i] = 0;
		// ghost code
		if (__CPROVER_same_object(demonic_array, a) && demonic_i == i) {
		    // update shadow count for a[i] if it is currently tracked
			count += 1;
		}

		if (i == 2) // assign twice the cell 2
		{
			a[i] = 0;
			// ghost code
			if (__CPROVER_same_object(demonic_array, a) && demonic_i == i) {
					// update shadow count for a[i] if it is currently tracked
				count += 1;
			}
		}
	}
```

We detect that some cell is assigned twice:
```bash
toto.c function main
[main.assertion.1] line 82 assertion demonic_array[demonic_i] == 0: SUCCESS
[main.assertion.2] line 84 assertion count == 1: FAILURE

State 24 file toto.c function main line 28 thread 0
----------------------------------------------------
  demonic_array=a!0@1 (00000010 00000000 00000000 00000000 00000000 00000000 00000000 00000000)

State 25 file toto.c function main line 29 thread 0
----------------------------------------------------
  demonic_i=2ul (00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000010) 

State 64 file toto.c function main line 63 thread 0
----------------------------------------------------
  count=2ul (00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000010)

Violated property:
  file toto.c function main line 84 thread 0
  assertion count == 1
  !((signed long int)(signed long int)!(count == (unsigned long int)1) != 0l)
```

So this shows that to enforce a global invariant on any memory location we can nondeterministically pick a single location, track updates made to that location along any execution path, and assert your property at the end.

On this very last example, I inlined the assertions we want to check at each ghost state update:

```C
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// this is how nondeterministic functions are declared
size_t __VERIFIER_nondet_size_t();

// the array we decide to monitor (ghost state)
int *demonic_array = NULL;

// the index in the array we decide to monitor (ghost state)
size_t demonic_i = 0;

// the count of number of writes for that monitored array cell (ghost state)
size_t count = 0;

#define A_SIZE 5
#define B_SIZE 10
int main()
{
	int a[A_SIZE];
	__CPROVER_havoc_object(a);

	// ghost code
	if (__VERIFIER_nondet_size_t())
	{
		// pick a cell in a to monitor
		demonic_array = a;
		demonic_i = __VERIFIER_nondet_size_t();
		__CPROVER_assume(demonic_i < A_SIZE);
		count = 0;
	}

	int b[B_SIZE];
	__CPROVER_havoc_object(b);

	// ghost code
	if (__VERIFIER_nondet_size_t())
	{
		// pick a cell in a to monitor
		demonic_array = b;
		demonic_i = __VERIFIER_nondet_size_t();
		__CPROVER_assume(demonic_i < B_SIZE);
		count = 0;
	}

	// the code does something to the array
	for (size_t i = 0; i < A_SIZE; i++)
	{
		a[i] = 0;
		// ghost code
		if (__CPROVER_same_object(demonic_array, a) && demonic_i == i)
		{
			// update shadow count for a[i] if it is currently tracked
			count += 1;
			assert(demonic_array[demonic_i] == 0); // will pass
			assert(count <= 1); // will pass
		}

		if (i == 2) // assign twice the cell 2
		{
			a[i] = 0;
			// ghost code
			if (__CPROVER_same_object(demonic_array, a) && demonic_i == i)
			{
				// update shadow count for a[i] if it is currently tracked
				count += 1;
				assert(demonic_array[demonic_i] == 0); // will pass
				assert(count <= 1); // will fail
			}
		}
	}

	// the code does something to the array
	for (size_t i = 0; i < B_SIZE; i++)
	{
		b[i] = 0;
		// ghost code
		if (__CPROVER_same_object(demonic_array, b) && demonic_i == i)
		{
			// update shadow count for b[i] if it is currently tracked
			count += 1;
			assert(demonic_array[demonic_i] == 0); // will pass
			assert(count <= 1); // will pass
		}
	}

	return 0;
}
```

We now get to see the falsification right where it happens:

```bash
** Results:
toto.c function main
[main.assertion.1] line 56 assertion demonic_array[demonic_i] == 0: SUCCESS
[main.assertion.2] line 57 assertion count <= 1: SUCCESS
[main.assertion.3] line 68 assertion demonic_array[demonic_i] == 0: SUCCESS
[main.assertion.4] line 69 assertion count <= 1: FAILURE
[main.assertion.5] line 83 assertion demonic_array[demonic_i] == 0: SUCCESS
[main.assertion.6] line 84 assertion count <= 1: SUCCESS

** 1 of 6 failed (2 iterations)
VERIFICATION FAILED
```

I this example we tracked memory at the level of a single int (4-bytes) because the property property was at the granularity of an `int`.

For stacked borrows, the granularity of the property is a single byte.
