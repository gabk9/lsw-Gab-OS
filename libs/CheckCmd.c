#include "utils.h"

#if !defined(_WIN32) && !defined(__linux__) && !defined(__APPLE__)
    #error "Operational system not recognized, terminating program!!"
#endif

double calc(double num1, char *operation, double num2) {

    if (strcmp(operation, "+") == 0)
        return num1 + num2;
    else if (strcmp(operation, "-") == 0)
        return num1 - num2;
    else if (strcmp(operation, "*") == 0)
        return num1 * num2;
    else if (strcmp(operation, "/") == 0) {
        if (!num2) {
            puts("Error: can't divide by 0!\n\n");
            return NAN;
        }

        return num1 / num2;
    } else if (strcmp(operation, "%") == 0) {
        if (!num2) {
            puts("Error: can't divide by 0!\n\n");
            return NAN;
        }
        
        return fmod(num1, num2);
    } else if (strcmp(operation, "^") == 0) {
        if (ceil(num1) != num1 || ceil(num2) != num2) {
            printf("Error: must be integers\n\n");
            return NAN;
        }

        return (int32_t)num1 ^ (int32_t)num2;
    }
    else if (strcmp(operation, "&") == 0) {
        if (ceil(num1) != num1 || ceil(num2) != num2) {
            printf("Error: must be integers\n\n");
            return NAN;
        }

        return (int32_t)num1 & (int32_t)num2;
    }
    else if (strcmp(operation, "|") == 0) {
        if (ceil(num1) != num1 || ceil(num2) != num2) {
            printf("Error: must be integers\n\n");
            return NAN;
        }

        return (int32_t)num1 | (int32_t)num2;
    }
    else if (strcmp(operation, "<") == 0)
        return num1 < num2;
    else if (strcmp(operation, ">") == 0)
        return num1 > num2;

    else if (strcmp(operation, "**") == 0) {
        if (num1 < 0 && floor(num2) != num2) {
            printf("Error: negative base with non-integer exponent\n\n");
            return NAN;
        } else
            return pow(num1, num2);
    }
    else if (strcmp(operation, "<<") == 0) {
        if (num2 < 0 || num2 >= sizeof(long long) * 8) {
            printf("Error: shift amount must be between 0 and %zu\n\n", sizeof(uint64_t) * 8 - 1);
            return NAN;
        }
        return (uint64_t)num1 << (uint64_t)num2;
    }
    else if (strcmp(operation, ">>") == 0) {
        if (num2 < 0 || num2 >= sizeof(long long) * 8) {
            printf("Error: shift amount must be between 0 and %zu\n\n", sizeof(int64_t) * 8 - 1);
            return NAN;
        }
        return (int64_t)num1 >> (int64_t)num2;
    }

    else if (strcmp(operation, "&&") == 0)
        return (num1 != 0 && num2 != 0);
    else if (strcmp(operation, "||") == 0)
        return (num1 != 0 || num2 != 0);
    else if (strcmp(operation, "<=") == 0)
        return num1 <= num2;
    else if (strcmp(operation, ">=") == 0)
        return num1 >= num2;
    else if (strcmp(operation, "!=") == 0)
        return num1 != num2;
    else if (strcmp(operation, "==") == 0)
        return num1 == num2;

    else {
        printf("Error: Unknown operator '%s'\n\n", operation);
        return NAN;
    }
}

void manCmd(char *instruction, const char **cmds, uint8_t isInsideBash) {

    if (!instruction) {
        printf("man: missing operand\nUse \"man man\" to check the manual\n");
        return;
    }

    trim(instruction);
    trimEnd(instruction);


    if (strcmp(instruction, cmds[0]) == 0) //! clear
        printf("'clear' clears your terminal's screen and its scrollback buffer\n");

    else if (strcmp(instruction, cmds[1]) == 0) //! exit
        printf("'exit' exit the terminal\n");

    else if (strcmp(instruction, cmds[2]) == 0) { //! echo
        printf("'echo' displays a line of text in the terminal or in a file\n\necho [STRING] <-- print [STRING]\n");
        printf("\necho [STRING] > [FILE NAME] <-- print [STRING] inside [FILE NAME]\n"
               "\necho [STRING] * [int: COUNT] <-- print [STRING] [COUNT] times\n"
               "\necho [STRING] * [int: COUNT] > [FILE NAME] <-- print [STRING] [COUNT] times inside [FILE NAME]\n");
    }

    else if (strcmp(instruction, cmds[3]) == 0) //! neofetch
        printf("'neofetch', a fast system info script\n");

    else if (strcmp(instruction, cmds[4]) == 0) //! updatehistory
        printf("'updatehistory' displays the update history of the terminal\n");

    else if (strcmp(instruction, cmds[5]) == 0) //! cmds
        printf("'cmds' displays the list of commands available\n");

    else if (strcmp(instruction, cmds[6]) == 0) //! cd
        printf("'cd' changes the working directory of the terminal\n");
        
    else if (strcmp(instruction, cmds[7]) == 0) { //! ls
        printf("'ls' list directory contents\n\nls [OPTION]\n\nOptions:\n");
        printf("\t'-a', '--all'   show hidden files or folders\n");
    }

    else if (strcmp(instruction, cmds[8]) == 0) //! man
        printf("'man' a interface to the system reference manuals\n\nman [COMMAND NAME...]\n");

    else if (strcmp(instruction, cmds[9]) == 0) //! whoami
        printf("'whoami' displays the user that you are currently logged-in\n");

    else if (strcmp(instruction, cmds[10]) ==  0) //! date
        printf("'date' displays the current date and time\n");

    else if (strcmp(instruction, cmds[11]) ==  0) //! pwd
        printf("'pwd' displays the current working directory\n");                

    else if (strcmp(instruction, cmds[12]) ==  0) //! mkdir
        printf("'mkdir' makes directories\n\nmkdir [FOLDER NAME...]\n");

    else if (strcmp(instruction, cmds[13]) ==  0) { //! rmdir
        printf("'rmdir' removes empty directories\n\nrmdir [OPTION] [FOLDER NAME...]\n\n");
        printf("Options:\n\t'-b', '--recycle-bin'   moves to recycle bin\n");
    }
    
    else if (strcmp(instruction, cmds[14]) ==  0) //! cat
        printf("'cat' displays the file content\n\ncat [FILE NAME]\n");

    else if (strcmp(instruction, cmds[15]) ==  0) { //! touch
        printf("'touch' displays a line of text in the terminal or in a file\n\ntouch [FILE NAME] <-- creates [FILE NAME]\n");
        printf("\ntouch [FILE NAME] < [STRING] <-- print the string into [FILE]\n"
               "\ntouch [FILE NAME] < [STRING] * [int: COUNT] <-- print the string in file [COUNT] times\n");
    }

    else if (strcmp(instruction, cmds[16]) ==  0) { //! rm
        printf("'rm' removes files or empty folders\n\nrm [OPTION...] [FILE/FOLDER NAME...]\n\nOptions:\n");
        printf("\t'-f', '--force'         removes without prompt\n");
        printf("\t'-i', '--interactive'   prompt before deletion (default)\n");
        printf("\t'-b', '--recycle-bin'   moves to recycle bin\n");
    }

    else if (strcmp(instruction, cmds[17]) ==  0) //! history
        printf("'history' displays the history of commands you used\n");

    else if (strcmp(instruction, cmds[18]) ==  0) { //! uname
        printf("'uname' displays system information\n\nuname [OPTION...]\n\nOptions:\n");
        printf("\t'-a', '--all'                print all the information, in the following order\n");
        printf("\t'-s', '--kernel-name'        print the kernel name (default)\n");
        printf("\t'-n', '--nodename'           print the network node hostname\n");
        printf("\t'-r', '--kernel-release'     print the kernel release\n");
        printf("\t'-v', '--kernel-version'     print the kernel version\n");
        printf("\t'-m', '--machine'            print the machine hardware name\n");
        printf("\t'-o', '--operating-system'   print the operating system\n'");
    } 

   else if (strcmp(instruction, cmds[19]) == 0) //! grep
        printf(
            "'grep' search for patterns in files\n\n"
            "grep [OPTION] [PATTERN] [FILE]\n\n"
            "Options:\n"
            "\t'-i', '--ignore-case'   ignore case distinctions when matching\n"
        );              

    else if (strcmp(instruction, cmds[20]) == 0) { //! bc
        printf("'bc' a simple calculator on the terminal, so far it only works with 2 numbers.\n\nbc [OPTION...]\n\nOptions:\n");
        printf("\t'-q', '--quiet'     will not print the initial text\n");
        printf("\t'-l', '--mathlib'   includes the mathlib header\n");
        printf("\nOperators:\n"
               "\t'+'    : Addition\n"
               "\t'-'    : Subtraction\n"
               "\t'*'    : Multiplication\n"
               "\t'**'   : Power (num1 raised to num2)\n"
               "\t'/'    : Division\n"
               "\t'%%'    : Modulus (remainder of division)\n"
               "\t'^'    : Bitwise XOR (1 if bits differ)\n"
               "\t'&'    : Bitwise AND (1 only if both bits are 1)\n"
               "\t'|'    : Bitwise OR  (1 if at least one bit is 1)\n"
               "\t'<<'   : Bitwise left shift (num1 shifted left num2 times)\n"
               "\t'>>'   : Bitwise right shift (num1 shifted right num2 times)\n"
               "\t'&&'   : Logical AND (true if both nonzero)\n"
               "\t'||'   : Logical OR  (true if at least one nonzero)\n"
               "\t'<'    : Less than (true if num1 < num2)\n"
               "\t'<='   : Less than or equal (true if num1 <= num2)\n"
               "\t'>'    : Greater than (true if num1 > num2)\n"
               "\t'>='   : Greater than or equal (true if num1 >= num2)\n"
               "\t'=='   : Equal to (true if num1 equals num2)\n"
               "\t'!='   : Not equal to (true if num1 differs from num2)\n"

               "\nFunctions: (only works if mathlib is turned on)\n"
               "\tclear, cls     : Clears the calculator scrollback\n"
               "\tmathlib        : Turns mathlib on/off\n"
               "\tscale(X)       : Sets the precision (number of digits after decimal)\n"
               "\tsqrt(X)        : Calculates the square root of X\n"
               "\troot(X, Y)     : Calculates the Y-th root of X\n"
               "\tsin(X)         : Calculates the sine of X\n"
               "\tcos(X)         : Calculates the cosine of X\n"
               "\ttan(X)         : Calculates the tangent of X\n"
               "\trad(X)         : Converts radian to degrees\n"
               "\tdeg(X)         : Converts degrees to radians\n"
               "\tln(X)          : Calculates the natural logarithm of X\n"
               "\tlog10(X)       : Calculates the base 10 logarithm of X\n"
               "\tlog2(X)        : Calculates the base 2 logarithm of X\n"
               "\tlog(X, Y)      : Calculates the base X logarithm of Y\n"
               "\tfloor(X)       : Rounds X down to the nearest integer\n"
               "\tceil(X)        : Rounds X up to the nearest integer\n"
               "\tround(X)       : Rounds X to the nearest integer\n"
               "\tfact(X)        : Calculates the factorial of X (X!)\n"
               "\tsum(X, Y, Z)   : Calculates the sum of all numeric values from X up to Y (inclusive), stepping by Z. If Z is not provided, it defaults to 1\n"
               "\ttrunc(X)       : Returns the integer part of a floating point number\n"
               "\trand(X, Y)     : Randomizes an integer between X and Y (inclusive)\n\t\t         keywords: RAND_MAX (not case sensitive)\n\n"
               "\trandf(X, Y)    : Randomizes a floating point number between X and Y (inclusive)\n\t\t         keywords: RAND_MAX (not case sensitive)\n\n"
               "\thex(X)         : Converts X to hexadecimal\n"
               "\toct(X)         : Converts X to octal\n"
               "\tbin(X)         : Converts X to binary\n"
               "\tfah(X)         : Converts X celsius to fahrenheit\n"
               "\tcel(X)         : Converts X fahrenheit to celsius\n"
               "\tmi(X)          : Converts X to miles\n"
               "\tkm(X)          : Converts X to kilometers\n"
               "\tlb(X)          : Converts X to pounds\n"
               "\tkg(X)          : Converts X to kilograms\n"

               "\nConstants:\n"
               "\tPI   : 3.1415... constant (not case sensitive)\n"
               "\tE    : 2.7182... constant (not case sensitive)\n"

               "\nSuffixes: (not case sensitive and only works for non hexadecimals)\n"
               "\tK   : 1.000               (1e+3)\n"
               "\tM   : 1.000.000           (1e+6)\n"
               "\tB   : 1.000.000.000       (1e+9)\n"
               "\tT   : 1.000.000.000.000   (1e+12)\n"
        );

    }

    else if (strcmp(instruction, cmds[21]) ==  0) //! drives
        printf("'drives' lists available drives\n");

    else if (strcmp(instruction, cmds[22]) ==  0) //! clearhistory
        printf("'clearhistory' clears history.txt\n");

    else if (strcmp(instruction, cmds[23]) ==  0) //! rand
        printf("'rand' generates an integer between 0 and RAND_MAX (inclusive)\n");

    else if (strcmp(instruction, cmds[24]) ==  0) { //! alias
        printf("'alias' creates shortcuts for the terminal\n\n");
        printf("alias [SHORTCUT NAME]='[COMMAND]'\n");
    }

    else if (strcmp(instruction, cmds[25]) ==  0) //! rename
        printf("'rename' renames folders or files\n\nrename [OLD NAME] [NEW NAME]\n");

    else if (strcmp(instruction, cmds[26]) ==  0 && isInsideBash) { //! bash
        printf("'bash' shows bash version information\n\nbash [OPTION]\n\nOptions:\n");
        printf("\t'-v', '--version'   show version information\n");
    }

    else if (strcmp(instruction, cmds[27]) ==  0) //! head
        printf("'head' print the first 10 lines of a file\n\nhead [FILE NAME]\n");

    else if (strcmp(instruction, cmds[28]) ==  0) //! tail
        printf("'tail' print the first 10 lines of a file starting from the bottom\n\ntail [FILE NAME]\n");

    else if (strcmp(instruction, cmds[29]) ==  0) //! lc
        printf("'lc' print the line count of a file\n\nlc [FILE NAME]...\n");
        
    else if (strcmp(instruction, cmds[30]) == 0) //! yes
        printf("'yes' output a string repeatedly until killed\n\nyes <-- print 'y' until killed\n\nyes [STRING] <-- print string until killed\n");

    else if (strcmp(instruction, cmds[31]) == 0) { //! sleep
        printf("'sleep' delay for a specified amount of time\n\nsleep [double: TIME]\n\nSuffixes:\n");
        printf("\t's'   seconds (default)\n");
        printf("\t'm'   minutes\n");
        printf("\t'h'   hours\n");
        printf("\t'd'   days\n");
    }

    else if (strcmp(instruction, cmds[32]) == 0) { //! randstr
        printf("'randstr' randomizes a random string, with 16bit max length\n\nrandstr [OPTION]\nOptions:\n");
        printf("\t'-l', --len   use it to set the length [-l=(int: size) / --len=(int: size)]\n");
    }

    else if (strcmp(instruction, cmds[33]) == 0) { //!rev
        printf("'rev' reverse strings\n\n");
        printf("rev <-- reads from input\n\n");
        printf("rev [SOURCE] <-- print [SOURCE] content reversed\n\n");
        printf("rev [SOURCE FILE] > [DESTINATION FILE] <-- writes [SOURCE FILE] content reversed inside [DESTINATION FILE]\n");
    }

    else
        printf("The manual for the '%s' command was not found!!\n", instruction);

}

void processCommand(char *input, char *args, const char **cmds, uint16_t cmdCount,
                    char **address, char *history_path,
                    char *data_folder, uint8_t isInsideBash) {
    char *temp = strdup(input);
    if (!temp) {
        perror("Error: strdup failed");
        return;
    }

   char *first_andand = find_andand_outside_quotes(temp);

    if (first_andand) {
        char *start = temp;
        
        while (true) {
            char *pos = find_andand_outside_quotes(start);

            char *segment;
            if (pos) {
                *pos = '\0';
                segment = start;
                start = pos + 2;
            } else {
                segment = start;
            }

            trim(segment);
            trimEnd(segment);

            if (*segment) {
                processCommand(segment, NULL,
                            cmds, cmdCount,
                            address, history_path,
                            data_folder, isInsideBash);
            }

            if (!pos)
                break;
        }

        SAFE_FREE(temp);
        return;
    }


    #ifdef _WIN32
        if (isalpha(temp[0]) && temp[1] == ':' && temp[2] == '\0') {
            if (!SetCurrentDirectory(temp)) {
                char path[4] = { temp[0], ':', '\\', '\0' };
                if (!SetCurrentDirectory(path))
                    printf("Error: Drive %c: not accessible or does not exist\n", temp[0]);
            }
            SAFE_FREE(*address);
            *address = getcwd(NULL, 0);
            return;
        }
    #endif

    char *instruction = extract_instruction(temp, &args);

    if (!instruction) {
        SAFE_FREE(temp);
        return;
    }

    trim(instruction);
    trimEnd(instruction);
    trimBetween(instruction);

    if (strcmp(instruction, cmds[0]) == 0) //! clear
        cls();

    else if (strcmp(instruction, cmds[1]) == 0) //! exit
        exit(0);

    else if (strcmp(instruction, cmds[2]) == 0) //! echo
        echoCmd(args ? args : "");

    else if (strcmp(instruction, cmds[3]) == 0) //! neofetch
        neofetchCmd();

    else if (strcmp(instruction, cmds[4]) == 0) //! updatehistory
        updatehistory();

    else if (strcmp(instruction, cmds[5]) == 0) //! cmds
        cmdsCommand(cmds, cmdCount, isInsideBash);

    else if (strcmp(instruction, cmds[6]) == 0) { //! cd
    #ifdef _WIN32           
        if (args) charReplace(args, '/', '\\');
    #else 
        if (args) charReplace(args, '\\', '/');
    #endif          

        if (args) {
            char *new_address = cdCmd(args, *address);
            SAFE_FREE(*address);
            *address = new_address;
        } else {
            char *new_address = get_default_address();
            SAFE_FREE(*address);
            *address = new_address;
        }
    }


    else if (strcmp(instruction, cmds[7]) == 0) //! ls
        lsCmd(args ? args : "", *address);

    else if (strcmp(instruction, cmds[8]) == 0) //! man
        manCmdMulti(args, cmds, isInsideBash);

    else if (strcmp(instruction, cmds[9]) == 0) //! whoami
        puts(get_user());

    else if (strcmp(instruction, cmds[10]) == 0) //! date
        puts(get_time(TIME_FMT));

    else if (strcmp(instruction, cmds[11]) == 0) //! pwd
        puts(*address);

    else if (strcmp(instruction, cmds[12]) == 0) //! mkdir
        mkdirCmd(args ? args : "");

    else if (strcmp(instruction, cmds[13]) == 0) //! rmdir
        rmdirCmd(args ? args : "");

    else if (strcmp(instruction, cmds[14]) == 0) //! cat
        catCmd(args ? args : "", EOF, "cat");

    else if (strcmp(instruction, cmds[15]) == 0) //! touch
        touchCmd(args ? args : "");

    else if (strcmp(instruction, cmds[16]) == 0) { //! rm
        char *argv_rm[MAX_ARGS];
        uint16_t argc_rm = 1;

        argv_rm[0] = "rm";

        uint16_t count = 0;
        char **list;
        if (args) {
            list = parseData(args, &count);
    
            for (uint16_t i = 0; i < count && argc_rm < MAX_ARGS; i++) {
                argv_rm[argc_rm++] = list[i];
            }
        }

        rmCmd(args ? argc_rm : 1, argv_rm);

        if (args) {
            for (uint16_t i = 0; i < count; i++)
                SAFE_FREE(list[i]);
            SAFE_FREE(list);
        }
    }

    else if (strcmp(instruction, cmds[17]) == 0) //! history
        historyCmd(history_path);

    else if (strcmp(instruction, cmds[18]) == 0) { //! uname
        char *argv_uname[MAX_ARGS];
        uint16_t argc_uname = 1;

        argv_uname[0] = "uname";

        if (args) {
            char *token = strtok(args, " ");
            while (token && argc_uname < MAX_ARGS) {
                argv_uname[argc_uname++] = token;
                token = strtok(NULL, " ");
            }
        }
        
        char *info = unameCmd(argc_uname, argv_uname);

        if (info)
            puts(info);
    }
    
    else if (strcmp(instruction, cmds[19]) == 0) //! grep
        grepCmd(args ? args : "");

    else if (strcmp(instruction, cmds[20]) == 0) { //! bc
        char *argv_bc[MAX_ARGS];
        uint16_t argc_bc = 1;

        argv_bc[0] = "bc";

        if (args) {
            char *token = strtok(args, " ");
            while (token && argc_bc < MAX_ARGS) {
                argv_bc[argc_bc++] = token;
                token = strtok(NULL, " ");
            }
        }

        bcCmd(argc_bc, argv_bc, cmds);
    }
    
    else if (strcmp(instruction, cmds[21]) == 0) //! drives
        listDrives();

    else if (strcmp(instruction, cmds[22]) == 0) //! clearhistory
        clearHistoryCmd(history_path);

    else if (strcmp(instruction, cmds[23]) == 0) //! rand
        printf("%" PRIu32 "\n", better_rand32());

    else if (strcmp(instruction, cmds[24]) == 0) //! alias
        createShortcut(args ? args : "", data_folder);

    else if (strcmp(instruction, cmds[25]) == 0) //! rename
        renameCmd(args ? args : "");

    else if (strcmp(instruction, cmds[26]) == 0 && isInsideBash) //! bash
        bashCmd(args ? args : "");

    else if (strcmp(instruction, cmds[27]) == 0) //! head
        catCmd(args ? args : "", 10, cmds[27]);
        
    else if (strcmp(instruction, cmds[28]) == 0) //! tail
        tailCmd(args ? args : "", 10);

    else if (strcmp(instruction, cmds[29]) == 0) { //! lc
        uint32_t lineCount = lcCmd(args ? args : "");
        if (lineCount != U32_NAN)
            printf("%d\n", lineCount);
    }

    else if (strcmp(instruction, cmds[30]) == 0) { //! yes
        if (args)
            echoHandler(args);
        while (true)
            puts(args ? args : "y"); 
    }

    else if (strcmp(instruction, cmds[31]) == 0) //! sleep
        sleepCmd(args ? args : "");

    else if (strcmp(instruction, cmds[32]) == 0) { //! randstr
        char *str = randstrCmd(args ? args : "");
        if (str)
            puts(str);

        SAFE_FREE(str);
    }

    else if (strcmp(instruction, cmds[33]) == 0) //! rev
        revCmd(args ? args : "");

    else if (!isalias(instruction, args ? args : "",
                    cmds, cmdCount, address,
                    history_path, data_folder, isInsideBash))
        printf("The command '%s' was not found!!\n", instruction);

    SAFE_FREE(temp);
}

double CheckFunc(char *operation, char **functions, const char *uniOps, const char **multiOps, bool mathlib) {
    char op[0x3] = {0};

    if (mathlib) {
        int16_t main_op = find_main_operator_full(
            operation,
            multiOps,
            uniOps,
            op
        );

        if (main_op > 0 && operation[main_op - 1] != '(') {
            char left[0x100], right[0x100];

            strncpy(left, operation, main_op);
            left[main_op] = '\0';

            strcpy(right, operation + main_op + strlen(op));

            double n1 = eval(left, mathlib);
            double n2 = eval(right, mathlib);

            return calc(n1, op, n2);
        }
    }

    if (strncmp(operation, functions[0], 5) == 0 && mathlib) //! scale()
        return s_scale(operation);
    else if (strncmp(operation, functions[1], 4) == 0 && mathlib) //! sqrt()
        return s_sqrt(operation);
    else if (strncmp(operation, functions[2], 3) == 0 && mathlib) //! sin()
        return s_sin(operation);
    else if (strncmp(operation, functions[3], 3) == 0 && mathlib) //! cos()
        return s_cos(operation);
    else if (strncmp(operation, functions[4], 3) == 0 && mathlib) //! tan()
        return s_tan(operation);
    else if (strncmp(operation, functions[5], 2) == 0 && mathlib) //! ln()
        return s_ln(operation);
    else if (strncmp(operation, functions[6], 5) == 0 && mathlib) //! log10()
        return s_log10(operation);
    else if (strncmp(operation, functions[7], 4) == 0 && mathlib) //! log2()
        return s_log2(operation);
    else if (strncmp(operation, functions[8], 3) == 0 && mathlib) //! log()
        return s_log(operation);
    else if (strncmp(operation, functions[9], 5) == 0 && mathlib) //! floor()
        return s_floor(operation);
    else if (strncmp(operation, functions[10], 4) == 0 && mathlib) //! ceil()
        return s_ceil(operation);
    else if (strncmp(operation, functions[11], 5) == 0 && mathlib) //! round()
        return s_round(operation);
    else if (strncmp(operation, functions[12], 4) == 0 && mathlib) { //! fact()
        uint64_t num = s_fact(operation);
        return (num != U64_NAN) ? (long double)num : NAN;
    }
    else if (strncmp(operation, functions[13], 4) == 0 && mathlib) //! sign()
        return s_sign(operation);
    else if (strncmp(operation, functions[14], 3) == 0 && mathlib) //! sum()
        return s_sum(operation);
    else if (strncmp(operation, functions[15], 3) == 0 && mathlib) //! rad()
        return s_rad(operation);
    else if (strncmp(operation, functions[16], 3) == 0 && mathlib) //! deg()
        return s_deg(operation);
    else if (strncmp(operation, functions[17], 5) == 0 && mathlib) //! trunc()
        return s_trunc(operation);
    else if (strncmp(operation, functions[18], 5) == 0 && mathlib) //! randf()
        return s_randFloat(operation);
    else if (strncmp(operation, functions[19], 3) == 0 && mathlib) //! fah()
        return s_fah(operation);
    else if (strncmp(operation, functions[20], 3) == 0 && mathlib) //! cel()
        return s_cel(operation);
    else if (strncmp(operation, functions[21], 4) == 0 && mathlib) //! root()
        return s_root(operation);
    else if (strncmp(operation, functions[22], 4) == 0 && mathlib) //! rand()
        return s_randInt(operation);
    else if (strncmp(operation, functions[23], 2) == 0 && mathlib) //! mi()
        return s_miles(operation);
    else if (strncmp(operation, functions[24], 2) == 0 && mathlib) //! km()
        return s_km(operation);
    else if (strncmp(operation, functions[25], 2) == 0 && mathlib) //! lb()
        return s_pounds(operation);
    else if (strncmp(operation, functions[26], 2) == 0 && mathlib) //! kg()
        return s_kg(operation);
    else if (strncmp(operation, "oct", 3) == 0 && mathlib) //! oct()
        return parse_double(operation, "oct");
    else if (strncmp(operation, "hex", 3) == 0 && mathlib) //! hex()
        return parse_double(operation, "hex");
    else if (strncmp(operation, "bin", 3) == 0 && mathlib) //! hex()
        return parse_double(operation, "bin");

    uint16_t op_pos = 0;

    for (uint16_t i = 0; operation[i]; i++) {
        for (uint16_t j = 0; multiOps[j]; j++) {
            uint16_t len = strlen(multiOps[j]);
            if (strncmp(&operation[i], multiOps[j], len) == 0) {
                strcpy(op, multiOps[j]);
                op_pos = i;
                goto op_found;
            }
        }

        if (strchr(uniOps, operation[i])) {
            op[0] = operation[i];
            op[1] = '\0';
            op_pos = i;
            goto op_found;
        }
    }

    printf("Error: Invalid expression\n");
    return U64_NAN;

op_found:
    {
        char buffer[0x100];
        strcpy(buffer, operation);

        buffer[op_pos] = '\0';

        char *num1 = buffer;
        char *num2 = buffer + op_pos + strlen(op);

        return calc(h_atof(num1), op, h_atof(num2));
    }
}