# LeetCode Batch Tester 

Load variables from leetcode sample data. And test them in batch.

This is a header-only library, just include `dataloader.h` to use.

## Sample usage

Using the solution tester to test.

You can just test the functions without a hand-craft link list content, all things are done by the framework!

```c++
#include <iostream>
#include <cmath>
#include <vector>
#include "dataloader.h"

// List node is defined by LeetCode problem.
//   1. Just inherit from LinkListConstructor<T>
struct ListNode : public LinkListConstructor<int> {
    int val;
    ListNode *next, *prev;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}

    // 2. Define virtual methods of interface, to make constructor work
    LL_DUAL(next, prev, val);
};

int size_of_list_list(ListNode* list) {
    list->printList();
    int i = 0;
    while (list != nullptr) {
        i++;
        list = list->next;
    }
    return i;
}


int main() {
    createSolutionTester(size_of_list_list, 0)
            .addTestCase("[1,2,3,4]", 4)
            .addTestCase("[1,2,3,4, 5]", 5)
            .test();
}
```

This output will be
```
====================================================
Checking case #0
----------------------------------------------------
[1, 2, 3, 4, ] 
----------------------------------------------------
Value:     4
Expected:  4
----------------------------------------------------
  Test case # 0: Passed
====================================================
Checking case #1
----------------------------------------------------
[1, 2, 3, 4, 5, ] 
----------------------------------------------------
Value:     5
Expected:  5
----------------------------------------------------
  Test case # 1: Passed
====================================================
 TESTING COMPLETED: 2 total, 2 passed, 0 failed.

```

## Usage

### 1. Using a DataLoader

First, I introduce data loader.

**Problem**

Find `k`-th max value in an array `A`.

**Sample input**
```
A = [1,2,3,4,5,6,7,8], k=4
```

**Sample output**
```
5
```

When debugging in local IDEs, it's a matter to load these data, especially when there
is Link Lists, Nested Arrays. 

With this library, you can directly copy & paste the sample in text and load in C++ code like this:
```c++
int main() {
    auto loader = "A = [1,2,3,4,5,6,7,8], k=4"_dl;
    int k = loader["k"];
    vector<int> A = loader["A"];
    int result = Solution().findKthMaxNumber(A, k);
    cout << result;
}
```

Specially, you can just comment out samples when testing for multiply samples with `//` or `/* */`.
for example:
```c++
auto loader = R"sample(
    //A = [1,2,3,4,5,6,7,8], k=4
    /*
      A = [1,2,3]
      k = 2
    */
    A = [1,2,3,4,5], k=3
)sample"_dl;

int k = loader["k"]; // k=3
vector<int> A = loader["A"]; // A = [1,2,3,4,5]
```

**Load a Linked List**

```c++
// List node is defined by LeetCode problem.
//   1. Just inherit from LinkListConstructor<T>
struct ListNode : public LinkListConstructor<int> {
    int val;
    ListNode *next, *prev;
    ListNode() : val(0), next(nullptr) {}
    ListNode(int x) : val(x), next(nullptr) {}
    ListNode(int x, ListNode *next) : val(x), next(next) {}

    // 2. Define virtual methods of interface, to make constructor work
    LL_NEXT(next)  // Define how to get next element
    LL_PREV(prev)  // Define how to get previous element
    LL_VALUE(val)  // Define how to get value

    // YOU can also use this
    // LL_DUAL(next, prev, val)   // to define a dual link list
    // LL_SINGLE(next, value)     // to define a single link list
};

int main() {
    DataLoader loader = "[1,2,3,4,5]"_dl;
    // 3. Use DataLoader to load a linked list, just assign to it!
    ListNode* L = loader;
    //    By the way, we also provided serval methods, to contruct different types of linked list explictly!
    ListNode* L1 = loader.asLinkedList<ListNode>();
    ListNode* L2 = loader.asLoopLinkedList<ListNode>();
    ListNode* L3 = loader.asDualLinkedList<ListNode>();
    ListNode* L4 = loader.asLoopDualLinkedList<ListNode>();
    ListNode* L5 = loader.asSingleOrDualLinkedList<ListNode>();
    L1->printList();
    L4->printReversedList();
}
```

The output is
```
[1, 2, 3, 4, 5, ]
[5, 4, 3, 2, 1, ]
```

### 2. Solution Tester

When testing multiply test cases, it's usually not convince to copy 
and paste use cases, esoecially while modify the code functionally. So 
you can use this `SolutionTester` to help you with it.

For example: 
```c
class Solution {
    .... // Your solution code here.
};


int main() {
    // 0. Declare a SoultionTester object
    SolutionTester ST;
    // 1. Add test cases in text
    ST.addTestCase(R"sample(
        s = "adceb"    // variable 's' in 0th place
        p = "***a**b"  // variable 'p' in 1st place
        true  // answer in 2nd place
    )sample");
    
    ST.addTestCase(R"sample(
        s = "acdcb"
        p = "a*c?b"
        true
    )sample");
    
    ST.addTestCase(R"sample(
        s = "cb"
        p = "?a"
        false
    )sample");
    
    ST.setCheckFn([] (DataLoader& loader) -> bool {
        string s = loader["s"].asString(); // access variable 's'
        string p = loader["p"].asString(); // access variable 'p'
        bool val = Solution().isMatch(s, p);  // get the result
        return val == loader[2].asInt(); // access the answer in 2nd place
                                         // then compare to check if it's a right answer.
    });
    ST.test();  // start testing
}
```

The result will tell you which test case has passed, and which case hasn't, shown below:

```
Checking case #0
----------------------------------------------------

----------------------------------------------------
  Test case # 0: Passed
====================================================
Checking case #1
----------------------------------------------------

----------------------------------------------------
  Test case # 1: Passed
====================================================
 TESTING COMPLETED: 2 total, 2 passed, 0 failed.
```

### 3. Using an Enhanced Solution Tester

#### **3.1 Using an EnhancedSolutionTester**

Instead of setting a checking function manually, use of `EnhancedSolutionTester` is much easier.

For example:

```c++
int test(vector<vector<int>> a, vector<int> b) {
    return a.size() + b.size();
}

int main() {
    EnhancedSoultionTester<int> ST;
    // add test case here
    ST.addTestCase(" a=[[1,2,3,4],[2,3,4,5]] b=[1,2,3,4]", 6);
    ST.addTestCase(" a=[[1,2,3,4],[2,3,4,5]] b=[1,2]", 4);
    
    // set check function
    ST.test(test, 0, "b");  // where 0 and "b" are the indexes of 
                            // the test cases in string (a.k.a, the DataLoader)
}
```

#### **3.2 Using a functional interface**

We provided a functional interface, You can use `createSolutionTester` if you do not want to make a soultion tester insance manually.

```c++
#include <iostream>
#include <cmath>
#include <vector>
#include "dataloader.h"

int test(vector<vector<int>> a, vector<int> b) {
    cout << a[0][1] << " " << b[1] << endl;
    return a[0][1] + b[1];
}

class Solution {
public:
    int test(vector<vector<int>> a, vector<int> b) {
        cout << a[0][1] << " " << b[1] << endl;
        return a[0][1] + b[1];
    }
};

int main() {
    // where &S::test is a class member function address, 0 and b are parameter indexes.
    createSolutionTester(&Solution::test, 0, "b")
            .addTestCase(" a=[[1,2,3,4],[2,3,4,5]] b=[1,2,3,4]", 4)
            .addTestCase(" a=[[1,2,3,4],[2,3,4,5]] b=[1,4]", "6"_dl)
            .test();

    // where test is the function name, 0 and b are parameter indexes.
    createSolutionTester(test, 0, "b")
            .addTestCase(" a=[[1,2,3,4],[2,3,4,5]] b=[1,2,3,4]", 4)
            .addTestCase(" a=[[1,2,3,4],[2,3,4,5]] b=[1,4]", "6"_dl)
            .test();
}
```
