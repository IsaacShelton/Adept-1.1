# Adept
## A blazing fast low-level language
Adept is designed for performace and simplicity.<br/>
Generally it runs as fast or faster than C code.

# Examples

### Variables
```
def main() int {
    age int = 36
    total int = 10 + 3
    return age + total
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
    hello *ubyte = malloc(12)
    
    // Set the byte array
    strcpy(hello, "Hello World")
    
    // Print the string
    puts(hello)
    
    // Free the dynamically allocated memory
    free(hello)
    return 0
}
```

### Arrays
```
public def main() int {
    // Allocate 10 ints (since an int is 32 bits)
    an_array *int = malloc(40)
    
    // Using arrays with pointer arithmetic
    *an_array = 10
    an_array[3] = 42
    an_array[7] = an_array[0] + 3
    
    // Free the dynamically allocated memory
    free(an_array)
    return 0
}
```

### Custom Builds
build.adept
```
private import "adept/build.adept"
private import "adept/system.adept"

private def build() int {
    config *adept.BuildConfig = adept.getBuildConfig()
    
    if adept.compile("main.adept") != 0 {
    	puts("Failed to compile 'main.adept'")
        return 1
    }
    
    puts("Successfully created 'main.exe'")
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
Compiling a program with a function named 'build' will run the program as a build script (using 'build' as the entry point)<br>
