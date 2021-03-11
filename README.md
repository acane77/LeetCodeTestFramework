# LeetCode Batch Tester 

Load variables from leetcode sample data. And test them in batch.

This is a header-only library, just include `dataloader.h` to use.

For example: 

## Usage

### 1. DataLoader

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
    DataLoader loader;
    loader.load(R"sample(
        A = [1,2,3,4,5,6,7,8], k=4
    )sample");
    int k = loader["k"];
    vector<int> A = loader["A"].asArray<int>();
    int result = Solution().findKthMaxNumber(A, k);
    cout << result;
}
```

Specially, you can just comment out samples when testing for multiply samples with `//` or `/* */`.
for example:
```c++
loader.load(R"sample(
    //A = [1,2,3,4,5,6,7,8], k=4
    /*
      A = [1,2,3]
      k = 2
    */
    A = [1,2,3,4,5], k=3
)sample");

int k = loader["k"]; // k=3
vector<int> A = loader["A"].asArray<int>(); // A = [1,2,3,4,5]
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
    LL_DEFINE_NEXT_PTR(next, ListNode)  // Define how to get next element
    LL_DEFINE_PREV_PTR(prev, ListNode)  // Define how to get previous element
    LL_DEFINE_VALUE(val, int)           // Define how to get value
};

int main() {
    DataLoader loader;
    loader.load("[1,2,3,4,5]");
    // 3. Use asLinkedList series methods to construct Link List
    ListNode* L1 = loader.asLinkedList<int, ListNode>();
    ListNode* L2 = loader.asLoopLinkedList<int, ListNode>();
    ListNode* L3 = loader.asDualLinkedList<int, ListNode>();
    ListNode* L4 = loader.asDualLoopLinkedList<int, ListNode>();
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
	-	a	d	c	e	b   $
-	1	0	0	0	0	0	0	
*	1	1	1	1	1	1	1	
*	1	1	1	1	1	1	1	
*	1	1	1	1	1	1	1	
a	0	1	0	0	0	0	0	
*	0	1	1	1	1	1	1	
*	0	1	1	1	1	1	1	
b	0	0	0	0	0	1	0	
$	0	0	0	0	0	0	1	
----------------------------------------------------
  Test case # 0: Passed
====================================================
Checking case #1
----------------------------------------------------
	-	a	c	d	c	b	$
-	1	0	0	0	0	0	0	
a	0	1	0	0	0	0	0	
*	0	1	1	1	1	1	1	
c	0	0	1	0	1	0	0	
?	0	0	0	1	0	1	0	
b	0	0	0	0	0	0	0	
$	0	0	0	0	0	0	0	
----------------------------------------------------
====================================================
Checking case #2
----------------------------------------------------
	-	c	b   $
-	1	0	0	0	
?	0	1	0	0	
a	0	0	0	0	
$	0	0	0	0	
----------------------------------------------------
  Test case # 2: Passed
====================================================
  Error while checking case #1
```
