
#ifndef DIE_H_INCLUDED
#define DIE_H_INCLUDED

#define fail(a) { std::cerr << a << std::endl; }

#define next_index(i, s) { if(++i == s) { fail("Unexpected Statement Termination"); return 1; } }

#endif // DIE_H_INCLUDED
