#define _GNU_SOURCE
#include "utils.h"

double Ans = NAN;

#if !defined(_WIN32) && !defined(__linux__) && !defined(__APPLE__) && !defined(__ANDROID__)
    #error "Operational system not recognized, terminating program!!"
#endif

char *stringToVariable(const char *str, int32_t *changed) {
    *changed = 0;

    if (strcasecmp(str, "$path") == 0) {
        *changed = 1;
        return get_env_var("PATH");
    }
    else if (strcasecmp(str, "$home") == 0) {
        *changed = 1;
        return get_env_var("HOME");
    }
    else if (strcasecmp(str, "$username") == 0 || strcasecmp(str, "$user") == 0) {
        *changed = 1;
        const char *user = get_user();
        return strdup(user);
    }
    else if (strcasecmp(str, "$temp") == 0) {
        *changed = 1;
        const char *tmp = getenv("TMP");
        if (!tmp) tmp = getenv("TEMP");
        if (!tmp) tmp = "/tmp";
        return strdup(tmp);
    }

#ifndef _WIN32
    else if (strcasecmp(str, "$shell") == 0) {
        *changed = 1;
        return get_env_var("SHELL");
    }
    else if (strcasecmp(str, "$lang") == 0) {
        *changed = 1;
        return get_env_var("LANG");
    }
    else if (strcasecmp(str, "$pwd") == 0) {
        *changed = 1;
        return get_env_var("PWD");
    }
    else if (strcasecmp(str, "$editor") == 0) {
        *changed = 1;
        return get_env_var("EDITOR");
    }

#else
    else if (strcasecmp(str, "$comspec") == 0) {
        *changed = 1;
        return get_env_var("COMSPEC");
    }
    else if (strcasecmp(str, "$systemroot") == 0) {
        *changed = 1;
        return get_env_var("SystemRoot");
    }
    else if (strcasecmp(str, "$appdata") == 0) {
        *changed = 1;
        return get_env_var("APPDATA");
    }
    else if (strcasecmp(str, "$localappdata") == 0) {
        *changed = 1;
        return get_env_var("LOCALAPPDATA");
    }
    else if (strcasecmp(str, "$programdata") == 0) {
        *changed = 1;
        return get_env_var("PROGRAMDATA");
    }
    else if (strcasecmp(str, "$public") == 0) {
        *changed = 1;
        return get_env_var("PUBLIC");
    }
    else if (strcasecmp(str, "$os") == 0) {
        *changed = 1;
        return get_env_var("OS");
    }
    else if (strcasecmp(str, "$number_of_processors") == 0) {
        *changed = 1;
        return get_env_var("NUMBER_OF_PROCESSORS");
    }
    else if (strcasecmp(str, "$processor_architecture") == 0) {
        *changed = 1;
        return get_env_var("PROCESSOR_ARCHITECTURE");
    }
#endif
    else if (str[0] == '$' && str[1] != '\0' && str[1] != ' ') {
        *changed = 1;
        return strdup("\n");
    }

    return strdup(str);
}

void checkLswrcSyntax(char *data_folder) {
    char *path = buildLswRcPath(data_folder);
    
    FILE *f = fopen(path, "r");

    if (!f)
        return;
    
    char line[MAX_CHAR];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';
        char *lineCpy = strdup(line);
        removeComments(line);
        trim(line);
        trimEnd(line);
        trimBetween(line);

        if (*line == '\0')
            continue;

        char *args;
        char *cmd = extractCommandOrKey(line, &args);

        char *secondCpy = strdup(lineCpy);

        if (strcmp(cmd, "alias") == 0) {
            char *eq = findFirstEqualOutsideQuotes(lineCpy);

            if (!eq) {
                fprintf(stderr, "alias: syntax error\n");
                SAFE_FREE(lineCpy);
                SAFE_FREE(secondCpy);
                SAFE_FREE(path);
                SAFE_FCLOSE(f);
                exit(EXIT_FAILURE);
            }

            *eq = '\0';
            char *shortcutName = lineCpy;
            char *action = eq + 1;

            shortcutName = strchr(shortcutName, ' ');
            
            trim(action); trimEnd(action);
            trim(shortcutName); trimEnd(shortcutName);
            
            if (!shortcutName) {
                fprintf(stderr, "alias: missing shortcut name\n");
                SAFE_FREE(lineCpy);
                SAFE_FREE(secondCpy);
                SAFE_FREE(path);
                SAFE_FCLOSE(f);
                exit(EXIT_FAILURE);
            }

            if (!action) {
                fprintf(stderr, "alias: missing action\n");
                SAFE_FREE(lineCpy);
                SAFE_FREE(secondCpy);
                SAFE_FREE(path);
                SAFE_FCLOSE(f);
                exit(EXIT_FAILURE);
            }

            if (!isBetweenQuotes(action)) {
                fprintf(stderr, "alias: the action should be between quotes, and it must be equal\n");
                SAFE_FREE(lineCpy);
                SAFE_FREE(secondCpy);
                SAFE_FREE(path);
                SAFE_FCLOSE(f);
                exit(EXIT_FAILURE);
            }

        } else if (strcmp(cmd, "HISTSIZE") == 0) {

            if (!args) {
                fprintf(stderr, "HISTFILE: missing arguments!\n");
                SAFE_FREE(lineCpy);
                SAFE_FREE(secondCpy);
                SAFE_FREE(path);
                SAFE_FCLOSE(f);
                exit(EXIT_FAILURE);
            }
            trim(args);
            trimEnd(args);
            
            if (args[0] == '=')
            args[0] = ' ';
            trim(args);

            double num = h_atof(args, false);

            if (!isalldigit(args) || num != (int64_t)num) {
                fprintf(stderr, "HISTFILE: arguments with invalid data type!\n");
                SAFE_FREE(lineCpy);
                SAFE_FREE(secondCpy);
                SAFE_FREE(path);
                SAFE_FCLOSE(f);
                exit(EXIT_FAILURE);
            }
            
            if (num < HISTSIZE_MIN || num > HISTSIZE_MAX) {
                fprintf(stderr, "HISTFILE: the argument must be between 10 and 10000 (inclusive)\n");
                SAFE_FREE(lineCpy);
                SAFE_FREE(secondCpy);
                SAFE_FREE(path);
                SAFE_FCLOSE(f);
                exit(EXIT_FAILURE);                
            }

            if (isKeyRepeated(data_folder, "HISTSIZE")) {
                fprintf(stderr, "HISTFILE: double key found, it should work but remove the extra one\n");
                SAFE_FREE(lineCpy);
                SAFE_FREE(secondCpy);
                SAFE_FREE(path);
                SAFE_FCLOSE(f);
                exit(EXIT_FAILURE);
            }

        } else {
            fprintf(stderr, "LSW: invalid key found in lswrc: '%s'\n", cmd);
            SAFE_FREE(lineCpy);
            SAFE_FREE(secondCpy);
            SAFE_FREE(path);
            SAFE_FCLOSE(f);
            exit(EXIT_FAILURE);
        }
        SAFE_FREE(lineCpy);
        SAFE_FREE(secondCpy);
    }

    SAFE_FREE(path);
    SAFE_FCLOSE(f);
}

double calc(double num1, char *operation, double num2, bool mathLib) {

    double result;

    if (strcmp(operation, "+") == 0)
        result = num1 + num2;
    else if (strcmp(operation, "-") == 0)
        result = num1 - num2;
    else if (strcmp(operation, "*") == 0)
        result = num1 * num2;
    else if (strcmp(operation, "/") == 0) {

        if (num2 == 0.0) {

            if (!mathLib) {
                printf("eval: can't divide by 0\n\n");
                return NAN;
            }

            if (num1 == 0.0)
                return 0.0;

            int32_t negative = signbit(num1) ^ signbit(num2);

            return negative ? -INFINITY : INFINITY;
        }

        result = num1 / num2;
    } else if (strcmp(operation, "%") == 0) {
        if (num2 == 0) {
            printf("eval: can't divide by 0\n\n");
            return NAN;
        }

        result = fmod(num1, num2);
    } else if (strcmp(operation, "^") == 0) {
        if ((int64_t)num1 != num1 || (int64_t)num2 != num2) {
            printf("eval: must be integers\n\n");
            return NAN;
        }

        result = (int64_t)num1 ^ (int64_t)num2;
    }
    else if (strcmp(operation, "&") == 0) {
        if ((int64_t)num1 != num1 || (int64_t)num2 != num2) {
            printf("eval: must be integers\n\n");
            return NAN;
        }

        result = (int64_t)num1 & (int64_t)num2;
    }
    else if (strcmp(operation, "|") == 0) {
        if ((int64_t)num1 != num1 || (int64_t)num2 != num2) {
            printf("eval: must be integers\n\n");
            return NAN;
        }

        result = (int64_t)num1 | (int64_t)num2;
    }
    else if (strcmp(operation, "<") == 0)
        result = num1 < num2;
    else if (strcmp(operation, ">") == 0)
        result = num1 > num2;

    else if (strcmp(operation, "**") == 0) {
        if (num1 < 0 && (int64_t)num2 != num2) {
            printf("eval: negative base with non-integer exponent\n\n");
            return NAN;
        }
        else
            result = pow(num1, num2);
    }

    else if (strcmp(operation, "^^") == 0) {

        if (num2 != (int64_t)num2) {
            printf("eval: tetration height must be an integer\n\n");
            return NAN;
        } else if (num2 < 0) {
            printf("eval: tetration height must be non-negative\n\n");
            return NAN;
        } else if (num1 == 0.0 && num2 == 0.0) {
            printf("eval: 0^^0 is undefined\n\n");
            return NAN;
        } else {

            result = tetration(num1, (int32_t)num2);

            if (isnan(result))
                printf("eval: invalid input for tetration\n\n");
        }
    }

    else if (strcmp(operation, "<<") == 0) {
        if (num2 < 0 || num2 >= sizeof(long long) * 8) {
            printf("eval: shift amount must be between 0 and %zu\n\n", sizeof(uint64_t) * 8 - 1);
            return NAN;
        }
        result = (uint64_t)num1 << (uint64_t)num2;
    }
    else if (strcmp(operation, ">>") == 0) {
        if (num2 < 0 || num2 >= sizeof(long long) * 8) {
            printf("eval: shift amount must be between 0 and %zu\n\n", sizeof(int64_t) * 8 - 1);
            return NAN;
        }

        result = (int64_t)num1 >> (int64_t)num2;
    }

    else if (strcmp(operation, "&&") == 0)
        return (num1 != 0 && num2 != 0);
    else if (strcmp(operation, "||") == 0)
        return (num1 != 0 || num2 != 0);
    else if (strcmp(operation, "<=") == 0)
        result = num1 <= num2;
    else if (strcmp(operation, ">=") == 0)
        result = num1 >= num2;
    else if (strcmp(operation, "!=") == 0)
        result = num1 != num2;
    else if (strcmp(operation, "==") == 0)
        result = fabs(num1 - num2) < EPS;


    else {
        printf("eval: Unknown operator '%s'\n\n", operation);
        return NAN;
    }

    if (isinf(result)) {
        printf("eval: numeric overflow (too large)\n\n");
        return NAN;
    }

    return result;
}

void manCmd(char *instruction, const char **cmds, uint8_t isInsideBash) {

    if (!instruction) {
        printf("man: missing operand\nUse \"man man\" to check the manual\n");
        return;
    }

    if (strcmp(instruction, cmds[0]) == 0) //! clear
        printf("'clear' clears your terminal's screen and its scrollback buffer\n");

    else if (strcmp(instruction, cmds[1]) == 0) //! exit
        printf("'exit' exit the terminal\n");

    else if (strcmp(instruction, cmds[2]) == 0) { //! echo
        printf("'echo' displays a line of text in the terminal or in a file\n");
        printf("\nModes:\n");
        printf("\t'>'    write\n");
        printf("\t'>>'   append\n");
        printf("\nUsage:\n");
        printf("\techo [STRING]                                     <-- print [STRING]\n");
        printf("\techo [STRING] [MODE] [FILE NAME]                  <-- print [STRING] inside [FILE NAME]\n");
        printf("\techo [STRING] * [int: COUNT]                      <-- print [STRING] [COUNT] times\n");
        printf("\techo [STRING] * [int: COUNT] [MODE] [FILE NAME]   <-- print [STRING] [COUNT] times inside [FILE NAME]\n");

        printf("\nEnvironment variables recognized: (not case sensitive)\n");
        printf("\t$PATH                     <-- system PATH\n");
        printf("\t$HOME                     <-- home directory (Linux)\n");
        printf("\t$USERNAME / $USER         <-- username (Windows/Linux)\n");
        printf("\t$TEMP                     <-- temporary folder\n");

    #ifndef _WIN32
        printf("\t$SHELL                    <-- default shell (Linux)\n");
        printf("\t$LANG                     <-- system language/locale\n");
        printf("\t$PWD                      <-- current directory\n");
        printf("\t$EDITOR                   <-- default editor\n");
    #else
        printf("\t$COMSPEC                  <-- command interpreter (cmd.exe)\n");
        printf("\t$SystemRoot               <-- Windows directory\n");
        printf("\t$APPDATA                  <-- user app data\n");
        printf("\t$LOCALAPPDATA             <-- user local app data\n");
        printf("\t$PROGRAMDATA              <-- system app data\n");
        printf("\t$PUBLIC                   <-- public folder\n");
        printf("\t$OS                       <-- always Windows_NT\n");
        printf("\t$NUMBER_OF_PROCESSORS     <-- CPU count\n");
        printf("\t$PROCESSOR_ARCHITECTURE   <-- CPU architecture\n");
    #endif
    }

    else if (strcmp(instruction, cmds[3]) == 0) //! neofetch
        printf("'neofetch', a fast system info script\n");

    else if (strcmp(instruction, cmds[4]) == 0) //! updatehistory
        printf("'logs' displays the update history of the terminal\n");

    else if (strcmp(instruction, cmds[5]) == 0) //! cmds
        printf("'cmds' displays the list of commands available\n");

    else if (strcmp(instruction, cmds[6]) == 0) //! cd
        printf("'cd' changes the working directory of the terminal\n\nUsage:\n\tcd [DIRECTORY]\n");

    else if (strcmp(instruction, cmds[7]) == 0) { //! ls
        printf("'ls' list directory contents\n\nUsage:\n\tls [OPTION]\n\nOptions:\n");
        printf("\t'-a', '--all'   show hidden files or folders\n");
    }

    else if (strcmp(instruction, cmds[8]) == 0) //! man
        printf("'man' a interface to the system reference manuals\n\nUsage:\n\tman [COMMAND NAME...]\n");

    else if (strcmp(instruction, cmds[9]) == 0) //! whoami
        printf("'whoami' displays the user that you are currently logged-in\n");

    else if (strcmp(instruction, cmds[10]) ==  0) //! date
        printf("'date' displays the current date and time\n");

    else if (strcmp(instruction, cmds[11]) ==  0) //! pwd
        printf("'pwd' displays the current working directory\n");                

    else if (strcmp(instruction, cmds[12]) ==  0) //! mkdir
        printf("'mkdir' makes directories\n\nUsage:\n\tmkdir [FOLDER NAME...]\n");

    else if (strcmp(instruction, cmds[13]) ==  0) { //! rmdir
        printf("'rmdir' removes empty directories\n\nUsage:\n\trmdir [OPTION...] [FOLDER NAME...]\n\n");
        printf("Options:\n"
            "\t'-b', '--recycle-bin'   moves to recycle bin\n"
            "\t'-e', '--erase'         removes completely\n"
        );
    }

    else if (strcmp(instruction, cmds[14]) ==  0) //! cat
        printf("'cat' displays the file content\n\nUsage:\n\tcat [FILE NAME...]\n");

    else if (strcmp(instruction, cmds[15]) ==  0) { //! touch
        printf("'touch' displays a line of text in the terminal or in a file\n\nUsage:\n");
        printf("\ttouch [FILE NAME]                             <-- creates [FILE NAME]\n"
            "\ttouch [FILE NAME] < [STRING]                  <-- print the string into [FILE]\n"
            "\ttouch [FILE NAME] < [STRING] * [int: COUNT]   <-- print the string in file [COUNT] times\n");
        
        printf("\nEnvironment variables recognized: (not case sensitive)\n");
        printf("\t$PATH                     <-- system PATH\n");
        printf("\t$HOME                     <-- home directory (Linux)\n");
        printf("\t$USERNAME / $USER         <-- username (Windows/Linux)\n");
        printf("\t$TEMP                     <-- temporary folder\n");

    #ifndef _WIN32
        printf("\t$SHELL                    <-- default shell (Linux)\n");
        printf("\t$LANG                     <-- system language/locale\n");
        printf("\t$PWD                      <-- current directory\n");
        printf("\t$EDITOR                   <-- default editor\n");
    #else
        printf("\t$COMSPEC                  <-- command interpreter (cmd.exe)\n");
        printf("\t$SystemRoot               <-- Windows directory\n");
        printf("\t$APPDATA                  <-- user app data\n");
        printf("\t$LOCALAPPDATA             <-- user local app data\n");
        printf("\t$PROGRAMDATA              <-- system app data\n");
        printf("\t$PUBLIC                   <-- public folder\n");
        printf("\t$OS                       <-- always Windows_NT\n");
        printf("\t$NUMBER_OF_PROCESSORS     <-- CPU count\n");
        printf("\t$PROCESSOR_ARCHITECTURE   <-- CPU architecture\n");
    #endif
    }

    else if (strcmp(instruction, cmds[16]) ==  0) { //! rm
        printf("'rm' removes files or empty folders\n\nUsage:\n\trm [OPTION...] [FILE/FOLDER NAME...]\n\nOptions:\n");
        printf("\t'-f', '--force'         removes without prompt\n"
            "\t'-i', '--interactive'   prompt before deletion (default)\n"
            "\t'-b', '--recycle-bin'   moves to recycle bin\n"
            "\t'-e', '--erase'         removes completely\n"
        );
    }

    else if (strcmp(instruction, cmds[17]) ==  0) //! history
        printf("'history' displays the history of commands you used\n\nUsage:\n\thistory [int: lines]\n");

    else if (strcmp(instruction, cmds[18]) ==  0) { //! uname
        printf("'uname' displays system information\n\nUsage:\n\tuname [OPTION...]\n\nOptions:\n");
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
            "Usage:\n\tgrep [OPTION] [PATTERN] [FILE]\n\n"
            "Options:\n"
            "\t'-i', '--ignore-case'   ignore case distinctions when matching\n"
        );              

    else if (strcmp(instruction, cmds[20]) == 0) { //! bc
        printf("'bc' a simple calculator on the terminal, apparently it works with more than 2 numbers, but without operand precedence\n\nUsage:\n\tbc [OPTION...]\n\nOptions:\n");
        printf("\t'-q', '--quiet'     will not print the initial text\n");
        printf("\t'-l', '--mathlib'   includes the mathlib header\n");
        printf("\nOperations:\n"
            "\t'+'    : Addition\n"
            "\t         Example: 2 + 3 = 5\n"
            "\n"
            "\t'-'    : Subtraction\n"
            "\t         Example: 10 - 4 = 6\n"
            "\n"
            "\t'*'    : Multiplication\n"
            "\t         Example: 6 * 7 = 42\n"
            "\n"
            "\t'/'    : Division\n"
            "\t         Example: 8 / 2 = 4\n"
            "\n"
            "\t'%%'    : Modulus (remainder of division)\n"
            "\t         Example: 10 %% 3 = 1\n"
            "\t         Note: it also works with single point precision numbers\n"
            "\n"
            "\t'**'   : Power (num1 raised to num2)\n"
            "\t         Example: 2 ** 5 = 32\n"
            "\n"
            "\t'^^'   : Tetration (num1 raised to itself num2 times)\n"
            "\t         Example: 2 ^^ 3 = 16\n"
            "\t         Explanation: 2^(2^2) = 16\n"
            "\n"
            "\t'^'    : Bitwise XOR\n"
            "\t         Example: 5 ^ 3 = 6\n"
            "\t         Explanation: Operates on individual bits\n"
            "\t         Explanation: Each bit is compared. Result bit is 1 if the bits are different\n"
            "\n"
            "\t'&'    : Bitwise AND\n"
            "\t         Example: 5 & 3 = 1\n"
            "\t         Explanation: Each bit is compared. Result bit is 1 only if both bits are 1\n"
            "\n"
            "\t'|'    : Bitwise OR\n"
            "\t         Example: 5 | 2 = 7\n"
            "\t         Explanation: Each bit is compared. Result bit is 1 if at least one bit is 1\n"
            "\n"
            "\t'~'    : Bitwise NOT\n"
            "\t         Example: ~2 = -3 and 100 ^ ~100 = -1\n"
            "\t         Explanation: Inverts every bit of the number\n"
            "\n"
            "\t'<<'   : Bitwise left shift\n"
            "\t         Example: 1 << 3 = 8\n"
            "\t         Explanation: 0b00001 << 3 = 0b01000\n"
            "\n"
            "\t'>>'   : Bitwise right shift\n"
            "\t         Example: 8 >> 2 = 2\n"
            "\t         Explanation: 0b01000 >> 2 = 0b00010\n"  
            "\n"
            "\t'&&'   : Logical AND\n"
            "\t         Example: 5 && 0 = 0\n"
            "\t         Note: Any non-zero value is treated as true\n"
            "\t         Explanation: Result is true(1) only if both operands are true(1), otherwise returns false(0)\n"
            "\n"
            "\t'||'   : Logical OR\n"
            "\t         Example: 0 || 5 = 1\n"
            "\t         Explanation: Result is true(1) if at least one operand is true(1), otherwise returns false(0)\n"
            "\n"
            "\t'<'    : Less than\n"
            "\t         Example: 2 < 5 = 1\n"
            "\t         Explanation: returns 1 if the number is less than the other, otherwise returns 0\n"
            "\n"
            "\t'<='   : Less than or equal\n"
            "\t         Example: 5 <= 5 = 1\n"
            "\t         Explanation: returns 1 if the number is less or equal than the other, otherwise returns 0\n"
            "\n"
            "\t'>'    : Greater than\n"
            "\t         Example: 8 > 3 = 1\n"
            "\t         Explanation: returns 1 if the number is greater than the other, otherwise returns 0\n"
            "\n"
            "\t'>='   : Greater than or equal\n"
            "\t         Example: 4 >= 4 = 1\n"
            "\t         Explanation: returns 1 if the number is greater or equal than the other, otherwise returns 0\n"
            "\n"
            "\t'=='   : Equal to\n"
            "\t         Example: 6 == 6 = 1\n"
            "\t         Explanation: returns 1 if the number is equal to the other, otherwise returns 0\n"
            "\n"
            "\t'!='   : Not equal to\n"
            "\t         Example: 6 != 5 = 1\n"
            "\t         Explanation: returns 1 if the number is different to the other, otherwise returns 0\n"


            "\nFunctions: (mathlib must be on to grant access)\n"
            "\tclear, cls     : Clears the calculator scrollback\n"
            "\n"
            "\tmathlib        : Toggles mathlib\n"
            "\t                 Explanation: turns on/off functions and other numeric systems\n"
            "\n"
            "\tscale(X)       : Sets decimal precision\n"
            "\t                 Example: scale(3.1415) = 4\n"
            "\t                 Explanation: Number of digits after decimal point\n"
            "\n"
            "\tsqrt(X)        : Square root\n"
            "\t                 Example: sqrt(16) = 4\n"
            "\n"
            "\troot(X, Y)     : X-th root of Y\n"
            "\t                 Example: root(3, 27) = 3\n"
            "\n"
            "\tsin(X)         : Sine of X\n"
            "\t                 Example: sin(rad(90)) = 1\n"
            "\t                 Note: X is in radians\n"
            "\n"
            "\tasin(X)        : Arc sine (inverse sine) of X\n"
            "\t                 Example: asin(0.5) = 0.523599\n"
            "\t                 Note: Returns value in radians\n"
            "\n"
            "\tcos(X)         : Cosine of X\n"
            "\t                 Example: cos(rad(0)) = 1\n"
            "\t                 Note: X is in radians\n"
            "\n"
            "\tacos(X)        : Arc cosine (inverse cosine) of X\n"
            "\t                 Example: acos(0.5) = 1.0472\n"
            "\t                 Note: Returns value in radians\n"
            "\n"
            "\ttan(X)         : Tangent of X\n"
            "\t                 Example: tan(rad(45)) = 1\n"
            "\t                 Note: X is in radians\n"
            "\n"
            "\tatan(X)        : Arc tangent (inverse tangent) of X\n"
            "\t                 Example: atan(1) = 0.785398\n"
            "\t                 Note: Returns value in radians\n"
            "\n"
            "\tcot(X)         : Cotangent of X\n"
            "\t                 Example: cot(rad(30)) = 1.73205\n"
            "\t                 Note: X is in radians\n"
            "\n"
            "\tacot(X)        : Arc cotangent (inverse cotangent) of X\n"
            "\t                 Example: acot(1) = 0.785398\n"
            "\t                 Note: Returns value in radians (range: 0 < result < pi)\n"
            "\n"
            "\trad(X)         : Degrees to radians\n"
            "\t                 Example: rad(3.1415) = 180\n"
            "\n"
            "\tdeg(X)         : Radians to degrees\n"
            "\t                 Example: deg(180) = 3.1415\n"
            "\n"
            "\tgon(X)         : Radians to gradians\n"
            "\t                 Example: gon(1) = 63.662\n"
            "\n"
            "\tln(X)          : Natural logarithm\n"
            "\t                 Example: ln(E) = 1\n"
            "\n"
            "\tlog10(X)       : Base-10 logarithm\n"
            "\t                 Example: log10(1000) = 3\n"
            "\n"
            "\tlog2(X)        : Base-2 logarithm\n"
            "\t                 Example: log2(8) = 3\n"
            "\n"
            "\tlog(X, Y)      : Logarithm of Y in base X\n"
            "\t                 Example: log(2, 32) = 5\n"
            "\n"
            "\tfloor(X)       : Rounds down\n"
            "\t                 Example: floor(3.7) = 3\n"
            "\n"
            "\tceil(X)        : Rounds up\n"
            "\t                 Example: ceil(3.2) = 4\n"
            "\n"
            "\tround(X)       : Rounds to nearest integer\n"
            "\t                 Example: round(3.5) = 4\n"
            "\n"
            "\tfact(X)        : Factorial\n"
            "\t                 Example: fact(5) = 120\n"
            "\t                 Note: Only defined for non-negative integers\n"
            "\n"
            "\tsum(X, Y, Z)   : Sum from X to Y with step Z\n"
            "\t                 Example: sum(1, 10, 2) = 25\n"
            "\t                 Note: If Z is omitted, step defaults to 1\n"
            "\n"
            "\ttrunc(X)       : Integer part of X\n"
            "\t                 Example: trunc(3.9) = 3\n"
            "\n"
            "\trand(X, Y)     : Random integer between X and Y\n"
            "\t                 Example: rand(1, 10)\n"
            "\n"
            "\trandf(X, Y)    : Random float between X and Y\n"
            "\t                 Example: randf(0, 1)\n"
            "\n"
            "\thex(X)         : Convert X to hexadecimal\n"
            "\t                 Example: hex(255) = 0x0FF\n"
            "\n"
            "\toct(X)         : Convert X to octal\n"
            "\t                 Example: oct(8) = 0o10\n"
            "\n"
            "\tbin(X)         : Convert X to binary\n"
            "\t                 Example: bin(5) = 0b0101\n"
            "\n"
            "\tchr(X)         : Convert X to ascii\n"
            "\t                 Example: ascii(65) = 'A'\n"
            "\t                 Note: escape characters does not work\n"
            "\t                 Tip: the number must be an integer between 0 and 127 (inclusive)\n"
            "\n"
            "\tfah(X)         : Celsius to Fahrenheit\n"
            "\t                 Example: fah(0) = 32\n"
            "\n"
            "\tcel(X)         : Fahrenheit to Celsius\n"
            "\t                 Example: cel(32) = 0\n"
            "\n"
            "\tmi(X)          : Kilometers to miles\n"
            "\t                 Example: mi(1) = 0.621\n"
            "\n"
            "\tkm(X)          : Miles to kilometers\n"
            "\t                 Example: km(1) = 1.609\n"
            "\n"
            "\tlb(X)          : Kilograms to pounds\n"
            "\t                 Example: lb(1) = 2.2046\n"
            "\n"
            "\tkg(X)          : Pounds to kilograms\n"
            "\t                 Example: kg(1) = 0.4535\n"
            "\n"
            "\tmeter(X)       : Converts feet (X) to meters\n"
            "\t                 Example: meter(5.8399) = 1.78\n"
            "\n"
            "\tfeet(X)        : Converts meter (X) to feet\n"
            "\t                 Example: feet(1.78) = 5.8399\n"
            "\n"
            "\tfabs(X)        : Absolute value (float)\n"
            "\t                 Example: fabs(-3.5) = 3.5\n"
            "\n"
            "\tabs(X)         : Absolute value (integer)\n"
            "\t                 Example: abs(-7) = 7\n"
            "\n"
            "\tlen(str)       : Returns the length of a string\n"
            "\t                 Example: strlen(\"string\") = 6\n"
            "\n"
            "\tbmi(X, Y)      : Returns your BMI with wight (X) in kg and height (Y) in meters\n"
            "\t                 Example: bmi(91, 1.78) = 28.7211\n"

            "\nBuiltin Variables: (mathlib must be on to grant access)\n"
            "\tAns   : stores the result of the last operation\n"
            "\t        Tip: initially set to NaN; it is also set to NaN after invalid operations\n"
            "\t        Note: its value cannot be changed manually\n"

            "\nConstants: (mathlib must be on to grant access)\n"
            "\tPI   : 3.141592...\n"
            "\t       Example: sin(PI / 2) = 1\n"
            "\n"
            "\tE    : 2.718281...\n"
            "\t       Example: ln(E) = 1\n"
            "\n"
            "\tINF  : 1.797e+308 (64 bit)\n"
            "\t     : Example: acot(-inf) = pi\n"

            "\nSuffixes: (only works for non hexadecimals and mathlib must be on to grant access)\n"
            "\tK   : 1.000               (1e+3)\n"
            "\t      Example: 5K = 5.000\n"
            "\n"
            "\tM   : 1.000.000           (1e+6)\n"
            "\t      Example: 3M = 3.000.000\n"
            "\n"
            "\tB   : 1.000.000.000       (1e+9)\n"
            "\t      Example: 2B = 2.000.000.000\n"
            "\n"
            "\tT   : 1.000.000.000.000   (1e+12)\n"
            "\t      Example: 1T = 1.000.000.000.000\n"

            "\nNumeric systems: (mathlib must be on to grant full access)\n"
            "\tBinary: (prefix: '0b')        base 2 numbers e.g. 0b010000000000 = 1024\n"
            "\n"
            "\tDecimal: (default):           base 10 numbers e.g. 1024\n"
            "\n"
            "\tOctal: (prefix: '0o')         base 8 numbers e.g. 0o2000 = 1024\n"
            "\n"
            "\tHexadecimal: (prefix: '0x')   base 16 numbers e.g. 0x400 = 1024\n"
            "\n"
            "\tAscii: (characters)           1 bytes chars only e.g. 'a' = 97\n"
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
        printf("Usage:\n\talias [SHORTCUT NAME]='[COMMAND]'\n");
    }

    else if (strcmp(instruction, cmds[25]) ==  0) //! rename
        printf("'rename' renames folders or files\n\nUsage:\n\trename [OLD NAME] [NEW NAME]\n");

    else if (strcmp(instruction, cmds[26]) ==  0 && isInsideBash) { //! bash
        printf("'bash' shows the shell information\n\nUsage:\n\tbash [OPTION...]\n\nOptions:\n");
        printf("\t'-v', '--version'   show version information\n"
            "\t'-h', '--help'      display manual\n"
            "\t'-a', '--all'       displays everything\n");
    }

    else if (strcmp(instruction, cmds[27]) ==  0) //! head
        printf("'head' print the first 10 lines of a file\n\nUsage:\n\thead [FILE NAME]\n");

    else if (strcmp(instruction, cmds[28]) ==  0) //! tail
        printf("'tail' print the first 10 lines of a file starting from the bottom\n\nUsage:\n\ttail [FILE NAME]\n");

    else if (strcmp(instruction, cmds[29]) ==  0) //! lc
        printf("'lc' displays the lines number of a file\n\nUsage:\n\tlc [FILE NAME...]\n");

    else if (strcmp(instruction, cmds[30]) == 0) { //! sleep
        printf("'sleep' delay for a specified amount of time\n\nUsage:\n\tsleep [double: TIME]\n\nSuffixes: (not case sensitive)\n");
        printf("\t's'   seconds (default)\n");
        printf("\t'm'   minutes\n");
        printf("\t'h'   hours\n");
        printf("\t'd'   days\n");
    }

    else if (strcmp(instruction, cmds[31]) == 0) { //! randstr
        printf("'randstr' randomizes a random string, with 16bit max length\n\nUsage:\n\trandstr [OPTION]\n\nOptions:\n");
        printf("\t'-l', --len   use it to set the length [-l=(int: size) / --len=(int: size)]\n");
    }

    else if (strcmp(instruction, cmds[32]) == 0) { //! rev
        printf("'rev' reverse strings\n\nUsage:\n");
        printf("\trev                                      <-- reads from input\n");
        printf("\trev [SOURCE]                             <-- print [SOURCE] content reversed\n");
        printf("\trev [SOURCE FILE] > [DESTINATION FILE]   <-- writes [SOURCE FILE] content reversed inside [DESTINATION FILE]\n");
    }

    else
        printf("man: the manual for the '%s' command was not found!!\n", instruction);

}

void processCommand(char *input, char *args, const char **cmds, uint16_t cmdCount,
                    char **address, char *history_path,
                    char *data_folder, bool isInsideBash, bool isFromAlias) {
    char *temp = strdup(input);
    if (!temp) {
        perror("bash: strdup failed");
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
                            data_folder, isInsideBash, false);
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
                    printf("bash: Drive %c: not accessible or does not exist\n", temp[0]);
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


    if (!isFromAlias && isalias(instruction, args ? args : "", cmds, cmdCount, address, 
    history_path, data_folder, isInsideBash)) {
        SAFE_FREE(temp);
        return;
    }

    if (strcmp(instruction, cmds[0]) == 0) //! clear
        cls();

    else if (strcmp(instruction, cmds[1]) == 0) //! exit
        exit(0);

    else if (strcmp(instruction, cmds[2]) == 0) //! echo
        echoCmd(args ? args : "");

    else if (strcmp(instruction, cmds[3]) == 0) { //! neofetch
        char *path = buildLswRcPath(data_folder);
        neofetchCmd(path);
    }

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


    else if (strcmp(instruction, cmds[7]) == 0) { //! ls
        uint16_t argc_ls;
        char **argv_ls = extract_args(args, &argc_ls, "ls");

        lsCmd(argv_ls, argc_ls, *address);

        for (uint16_t i = 1; i < argc_ls; i++)
            SAFE_FREE(argv_ls[i]);
        SAFE_FREE(argv_ls);
    }

    else if (strcmp(instruction, cmds[8]) == 0) //! man
        manCmdMulti(args ? args : "", cmds, isInsideBash);

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
        uint16_t argc_rm;
        char **argv_rm = extract_args(args, &argc_rm, "rm");

        rmCmd(args ? argc_rm : 1, argv_rm);

        if (args) {
            for (uint16_t i = 1; i < argc_rm; i++)
                SAFE_FREE(argv_rm[i]);
            SAFE_FREE(argv_rm);
        }
    }

    else if (strcmp(instruction, cmds[17]) == 0) //! history
        historyCmd(args ? args : "", history_path);

    else if (strcmp(instruction, cmds[18]) == 0) { //! uname
        uint16_t argc_uname;
        char **argv_uname = extract_args(args, &argc_uname, "uname");
        
        char *info = unameCmd(argc_uname, argv_uname);

        if (info)
            puts(info);

        if (args) {
            for (uint16_t i = 1; i < argc_uname; i++)
                SAFE_FREE(argv_uname[i]);
            SAFE_FREE(argv_uname);
        }   
    }

    else if (strcmp(instruction, cmds[19]) == 0) //! grep
        grepCmd(args ? args : "");

    else if (strcmp(instruction, cmds[20]) == 0) { //! bc
        uint16_t argc_bc;
        char **argv_bc = extract_args(args, &argc_bc, "bc");

        bcCmd(argc_bc, argv_bc, cmds);
        if (args) {
            for (uint16_t i = 1; i < argc_bc; i++)
                SAFE_FREE(argv_bc[i]);
            SAFE_FREE(argv_bc);
        }   
    }

    else if (strcmp(instruction, cmds[21]) == 0) //! drives
        listDrives();

    else if (strcmp(instruction, cmds[22]) == 0) //! clearhistory
        clearHistoryCmd(history_path);

    else if (strcmp(instruction, cmds[23]) == 0) //! rand
        printf("%u\n", better_rand32());

    else if (strcmp(instruction, cmds[24]) == 0) //! alias
        createShortcut(args ? args : "", data_folder);

    else if (strcmp(instruction, cmds[25]) == 0) //! rename
        renameCmd(args ? args : "");

    else if (strcmp(instruction, cmds[26]) == 0 && isInsideBash) { //! bash
        uint16_t argc_bash;
        char **argv_bash = extract_args(args, &argc_bash, "bash");

        bashCmd(argc_bash, argv_bash, cmds, cmdCount, true);

        if (args) {
            for (uint16_t i = 1; i < argc_bash; i++)
                SAFE_FREE(argv_bash[i]);
            SAFE_FREE(argv_bash);
        }
    }

    else if (strcmp(instruction, cmds[27]) == 0) //! head
        catCmd(args ? args : "", 10, cmds[27]);

    else if (strcmp(instruction, cmds[28]) == 0) //! tail
        tailCmd(args ? args : "", 10);

    else if (strcmp(instruction, cmds[29]) == 0) { //! lc
        uint32_t lineCount = lcCmd(args ? args : "");
        if (lineCount != U32_NAN)
            printf("%d\n", lineCount);
    }

    else if (strcmp(instruction, cmds[30]) == 0) //! sleep
        sleepCmd(args ? args : "");

    else if (strcmp(instruction, cmds[31]) == 0) { //! randstr
        char *str = randstrCmd(args ? args : "");
        if (str)
            puts(str);

        SAFE_FREE(str);
    }

    else if (strcmp(instruction, cmds[32]) == 0) //! rev
        revCmd(args ? args : "");

    else
        printf("bash: the command '%s' was not found!!\n", instruction);

    SAFE_FREE(temp);
}

double CheckOperation(char *operation, char **functions, const char *uniOps, const char **multiOps, bool mathlib) {
    char op[0x4] = {0};

    int16_t op_pos = find_main_operator_full(
        operation,
        multiOps,
        uniOps,
        op
    );

    if (op_pos == -1) {

        if (strcasecmp(operation, OLD_ANSWER_STR) == 0) {
            if (isnan(Ans)) {
                puts("Warning: Ans is undefined");
                return (double)U64_NAN;
            }

            return Ans;
        }

        if (mathlib) {
            char *test = strdup(operation);
            charRm(test, ' ');

            if (test[5] == '(' && strncmp(operation, functions[0], 5) == 0) { //! scale()
                SAFE_FREE(test);
                return s_scale(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[1], 4) == 0) { //! sqrt()
                SAFE_FREE(test);
                return s_sqrt(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[2], 3) == 0) { //! sin()
                SAFE_FREE(test);
                return s_sin(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[3], 3) == 0) { //! cos()
                SAFE_FREE(test);
                return s_cos(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[4], 3) == 0) { //! tan()
                SAFE_FREE(test);
                return s_tan(operation);
            } else if (test[2] == '(' && strncmp(operation, functions[5], 2) == 0) { //! ln()
                SAFE_FREE(test);
                return s_ln(operation);
            } else if (test[5] == '(' && strncmp(operation, functions[6], 5) == 0) { //! log10()
                SAFE_FREE(test);
                return s_log10(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[7], 4) == 0) { //! log2()
                SAFE_FREE(test);
                return s_log2(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[8], 3) == 0) { //! log()
                SAFE_FREE(test);
                return s_log(operation);
            } else if (test[5] == '(' && strncmp(operation, functions[9], 5) == 0) { //! floor()
                SAFE_FREE(test);
                return s_floor(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[10], 4) == 0) { //! ceil()
                SAFE_FREE(test);
                return s_ceil(operation);
            } else if (test[5] == '(' && strncmp(operation, functions[11], 5) == 0) { //! round()
                SAFE_FREE(test);
                return s_round(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[12], 4) == 0) { //! fact()
                SAFE_FREE(test);
                return s_fact(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[13], 4) == 0) { //! sign()
                SAFE_FREE(test);
                return s_sign(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[14], 3) == 0) { //! sum()
                SAFE_FREE(test);
                return s_sum(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[15], 3) == 0) { //! rad()
                SAFE_FREE(test);
                return s_rad(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[16], 3) == 0) { //! deg()
                SAFE_FREE(test);
                return s_deg(operation);
            } else if (test[5] == '(' && strncmp(operation, functions[17], 5) == 0) { //! trunc()
                SAFE_FREE(test);
                return s_trunc(operation);
            } else if (test[5] == '(' && strncmp(operation, functions[18], 5) == 0) { //! randf()
                SAFE_FREE(test);
                return s_randFloat(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[19], 3) == 0) { //! fah()
                SAFE_FREE(test);
                return s_fah(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[20], 3) == 0) { //! cel()
                SAFE_FREE(test);
                return s_cel(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[21], 4) == 0) { //! root()
                SAFE_FREE(test);
                return s_root(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[22], 4) == 0) { //! rand()
                SAFE_FREE(test);
                return s_randInt(operation);
            } else if (test[2] == '(' && strncmp(operation, functions[23], 2) == 0) { //! mi()
                SAFE_FREE(test);
                return s_miles(operation);
            } else if (test[2] == '(' && strncmp(operation, functions[24], 2) == 0) { //! km()
                SAFE_FREE(test);
                return s_km(operation);
            } else if (test[2] == '(' && strncmp(operation, functions[25], 2) == 0) { //! lb()
                SAFE_FREE(test);
                return s_pounds(operation);
            } else if (test[2] == '(' && strncmp(operation, functions[26], 2) == 0) { //! kg()
                SAFE_FREE(test);
                return s_kg(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[27], 3) == 0) { //! oct()
                SAFE_FREE(test);
                return parse_double(operation, functions[27]);
            } else if (test[3] == '(' && strncmp(operation, functions[28], 3) == 0) { //! hex()
                SAFE_FREE(test);
                return parse_double(operation, functions[28]);
            } else if (test[3] == '(' && strncmp(operation, functions[29], 3) == 0) { //! hex()
                SAFE_FREE(test);
                return parse_double(operation, functions[29]);
            } else if (test[3] == '(' && strncmp(operation, functions[30], 3) == 0) { //! abs()
                SAFE_FREE(test);
                return s_fabs_or_abs(operation, false);
            } else if (test[4] == '(' && strncmp(operation, functions[31], 4) == 0) { //! fabs()
                SAFE_FREE(test);
                return s_fabs_or_abs(operation, true);
            } else if (test[3] == '(' && strncmp(operation, functions[32], 3) == 0) { //! len()
                SAFE_FREE(test);
                return bc_len(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[33], 3) == 0) { //! bmi()
                SAFE_FREE(test);
                return s_bmi(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[34], 4) == 0) { //! feet()
                SAFE_FREE(test);
                return s_feet(operation);
            } else if (test[5] == '(' && strncmp(operation, functions[35], 5) == 0) { //! meter()
                SAFE_FREE(test);
                return s_meter(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[36], 3) == 0) { //! cot()
                SAFE_FREE(test);
                return s_cot(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[37], 3) == 0) { //! gon()
                SAFE_FREE(test);
                return s_gon(operation);
            } else if (test[3] == '(' && strncmp(operation, functions[38], 3) == 0) { //! chr()
                SAFE_FREE(test);
                return parse_double(operation, functions[38]);
            } else if (test[4] == '(' && strncmp(operation, functions[39], 4) == 0) { //! asin()
                SAFE_FREE(test);
                return s_asin(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[40], 4) == 0) { //! acos()
                SAFE_FREE(test);
                return s_acos(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[41], 4) == 0) { //! atan()
                SAFE_FREE(test);
                return s_atan(operation);
            } else if (test[4] == '(' && strncmp(operation, functions[42], 4) == 0) { //! acot()
                SAFE_FREE(test);
                return s_acot(operation);
            }

            SAFE_FREE(test);    

            return h_atof(operation, mathlib);
        }
    }

    char buffer[0x100];
    strcpy(buffer, operation);

    buffer[op_pos] = '\0';

    char *num1 = buffer;
    char *num2 = buffer + op_pos + strlen(op);

    trim(num1); trimEnd(num1);
    trim(num2); trimEnd(num2);

    if (isBcVariable(num1) || isBcVariable(num2)) {
        printf("Warning: variables are currently unsupported\n\n");
        return NAN;
    }

    double num1_double = eval(num1, mathlib);

    if (num1_double == (double)U64_NAN)
        return (double)U64_NAN;
    else if (num1_double == QUICK_EVAL_FIX)
        return 0.0;

    double num2_double = eval(num2, mathlib);

    if (num2_double == (double)U64_NAN)
        return (double)U64_NAN;
    else if (num2_double == QUICK_EVAL_FIX)
        return 0.0;

    return calc(num1_double, op, num2_double, mathlib);
}