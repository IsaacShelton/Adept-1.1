# The Adept programming language
## A blazing fast low-level language
Adept is designed for performace and simplicity.<br/>
Generally it runs as fast or faster than C code.<br/>
![adept_logo](https://github.com/IsaacShelton/Adept/blob/master/res/adept.png)<br/>

# Examples

### Variables
```
def main() int {
    customer_age int = 36
    total_amount int = 10 + 3
    return 0
}
```


### Functions
```
def sum(a int, b int) int {
    return a + b
}
def main() int {
    return sum(13, 8)
}
 ```

### Constants
```
private import "system/system.adept"
private constant $MESSAGE "Hello World"
 
public def main() int {
    puts($MESSAGE)
    return 0
}
```

### Pointers
```
private import "system/string.adept"
private import "system/system.adept"

public def main() int {
    // Allocate 12 bytes (one extra for null termination)
    hello *ubyte = malloc(12ui)
    
    // Set the byte array
    strcpy(hello, "Hello World")
    
    // Print the string
    puts(hello)
    
    // Free the dynamically allocated memory
    free(hello)
    return 0
}
```

### Higher-Level Arrays
```
private import "adept/terminal.adept"

public def main() int {
    // Create an array
    integer_array [] int = new [3] int
    
    // Set some elements of the array
    integer_array[0] = 10
    integer_array[1] = 11
    integer_array[2] = 12

    // Print the length of the array
    println(integer_array.length)
    
    // Print some elements of the array
    println(integer_array[0])
    println(integer_array[1])
    println(integer_array[2])
    
    // Free the array data
    delete integer_array.data
    return 0
}
```

### Low-Level Arrays
```
private import "adept/terminal.adept"

public def main() int {
    // Allocate 10 ints
    an_array *int = new int * 10
    
    // Using low-level arrays with basic pointer arithmetic
    *an_array = 10
    an_array[3] = 42
    an_array[7] = an_array[0] + 3
    
    // Print some of the values we set
    println(*an_array)
    println(an_array[3])    
    println(an_array[7])
    
    // Free the dynamically allocated memory
    delete an_array
    return 0
}
```

### Custom build scripts
NOTE: I did not come up with this concept myself, I got it from [Johnathan Blow](https://www.youtube.com/user/jblow888)'s programming language Jai. <br/><br/>
build.adept
```
private import "adept/terminal.adept"

private def build() int {
    config AdeptConfig
    
    config.create()
    defer config.free()
    
    config.setTiming(true)
    config.setOptimization(3ui)

    if config.compile("main.adept") != 0 {
        println("Failed to compile main.adept")
        return 1
    }
    
    println("Build Complete!")
    return 0
}

```
main.adept
```
public def main() int {
    some int = 10
    another int = 13
    some_more(code, here)
    return 0
}
```
Adept treats any file named ```build.adept``` as a just-in-time build script. When Adept invokes a build script, it uses the 'build' function as an entry point instead of 'main'. So to use ```build.adept``` to compile ```main.adept``` you run the command ```adept build.adept``` or just ```adept```<br>

### Using functions as operators
NOTE: I did not come up with this concept myself, I got it from [Johnathan Blow](https://www.youtube.com/user/jblow888)'s programming language Jai.
```
private import "adept/terminal.adept"

public def main() int {
    result int = 13 sum 8 sum 100
    println(result)
    return 0
}

private def sum(a int, b int) int {
    return a + b
}

```

### Multiple functions with the same name
```
public def main() int {
    int_sum int = sum(8, 13)
    long_sum long = sum(8sl, 13sl)
    
    println(int_sum)
    println(long_sum)
    return 0
}

private def sum(a int, b int) int {
    return a + b
}

private def sum(a long, b long) long {
    return a + b
}
```

### Classes and Object Oriented Programming
```
private import "adept/terminal.adept"

private class Vector3f {
    public x float
    public y float
    public z float
    
    public def set(x float, y float, z float) void {
        this.x = x
        this.y = y
        this.z = z
    }
    
    public def println() void {
        println(this.x)
        println(this.y)
        println(this.z)
    }
}

public def main() int {
    vector Vector3f
    vector.set(10.0f, 9.0f, 8.0f)
    vector.println()
    return 0
}
```

### Function pointers
```
private import "adept/terminal.adept"

public def add(a int, b int) int {
    // Basic function to add 2 integers
    return a + b
}

public def mul(a int, b int) int {
    // Basic function to multiply 2 integers
    return a * b
}

public def printcalc(calc def(int, int) int, a int, b int) void {
    // A simple function that preforms a calculation on 2 integers and prints the result
    println(calc(a, b))
}

public def main() int {
    // Declare a function pointer that points to 'add'
    sum def(int, int) int = funcptr add(int, int)
    
    // Call the function pointed to by 'sum'
    println(8 sum 13)
    
    // Pass it to a function and have the function call it
    println("--------------------------")
    printcalc(funcptr add(int,int), 8, 13)
    printcalc(funcptr mul(int,int), 8, 13)
    
    return 0
}
```

### Sizeof, new, and delete
```
private import "system/system.adept"
private import "adept/terminal.adept"

private type SmallType {
    length usize
    data ptr
}

private type BigType {
    a int
    b int
    c int
    d long
    e SmallType
}

public def main() int {
    // Get the size of the type 'BigType' and print it
    // NOTE: This will be different depending on the alignment of the type and
    //   depending on what the target system is. You can ensure a type is aligned to
    //   one byte by specifing the 'packed' attribute
    length usize = sizeof BigType
    println(cast int(length))
    
    // Use 'new' operator to allocate an object of type 'BigType' on the heap
    data *BigType = new BigType
    
    // Use the allocated object
    data.c = 12345
    println(data.c)
    
    // Use 'delete' statement to free the object
    delete data
    return 0
}
```

### Constant data arrays
```
private import "adept/terminal.adept"

public def main() int {
    integer_list *int = {
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
    }
    
    i usize = 0ui
    while i != 11ui {
        println(integer_list[i])
        i = i + 1ui
    }
    
    // Don't delete the data pointed to by
    //   'integer_list' because it is constant
    return 0
}
```
