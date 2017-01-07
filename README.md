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

### Custom build scripts
build.adept
```
private import "adept/build.adept"
private import "system/system.adept"

private def build() int {
	config adept\BuildConfig = adept\config()
	
	config.time = true
	config.optimization = $ADEPT\OPTIMIZATION_HIGH

	unless adept\compile("main.adept", &config) == 0 {
		return 1
	}
	
	puts("Build Complete!")
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
Adept treats any program with a function named 'build' as a just-in-time build script. When Adept invokes a build script, it uses the 'build' function as an entry point instead of 'main'. So to use ```build.adept``` to compile ```main.adept``` you just run the command ```adept build.adept```<br>

### Using functions as operators
```
private import "system/system.adept"
private import "adept/conversion.adept"

public def main() int {
    result int = 13 sum 8 sum 100
    
    message *ubyte = malloc(100)
    inttostr(result, message, 10)
    puts(message)
    free(message)
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
    return 0
}

private def sum(a int, b int) int {
    return a + b
}

private def sum(a long, b long) long {
    return a + b
}
```
