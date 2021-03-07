# LeetCode Input Sample Loader

Load varibles from leetcode sample data.

This is a header-only library, just include `dataloader.h` to use.

For example: 

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

