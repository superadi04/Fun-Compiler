# Fun-Interpreter

"Fun", a simple functional, turing-complete, pythonic, interpreted language with support for conditionals, functions, recursion types, arrays, and co-routines. Below are some examples:

## Conditionals
```python
integer n = 90

if (n % 2 == 0) {
    print("The number is even.")
} else {
    print("The number is odd.")
}
```

## For Loops

```python
integer f[n+2]
f[0] = 0
f[1] = 1
for(integer i = 2; i<=n; i = i + 1){
    if[i] = f[i-1] + f[i-2]
}
print(f[n])
```

## While Loops
```python
boolean test = true
integer i = 0

while(test) {
    i = i + 1
    
    if (i > 10) {
        test = !test
    }
}
```

## Functions

```python
integer[10] array

array[0] = 10
array[1] = 32
array[2] = 100
array[3] = 1231
array[4] = 43242
array[5] = 342242
array[6] = 2343244
array[7] = 2888899
array[8] = 28888990
array[9] = 909009000

fun binarySearchRecursive(key) {
    integer last = 9
    integer first = 0

    if (last>=first){  
        int mid = first + (last - first) / 2;  

        if (arr[mid] == key ){  
            return true;  
        }  
        if (arr[mid] > key) {  
            return binarySearch(arr, first, mid-1, key)
        } else {  
            return binarySearch(arr, mid+1, last, key)
        }  
    }  

    return false;  
}
```
