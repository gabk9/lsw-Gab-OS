#define _GNU_SOURCE
#include "utils.h"
#include "types.h"

var Ans = { .type = BC_NONE };

#if !defined(_WIN64) && !defined(__linux__) && !defined(__APPLE__) && !defined(__ANDROID__)
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

#ifndef _WIN64
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

void checkLswrcSyntax(const char *data_folder) {
    char *path = buildLswRcPath(data_folder);
    FILE *f = fopen(path, "r");

    if (!f) {
        SAFE_FREE(path);
        return;
    }

    char line[MAX_CHAR];

    size_t lineC = 0;

    while (fgets(line, sizeof(line), f)) {
        lineC++;

        line[strcspn(line, "\n")] = '\0';

        char lineOrig[MAX_CHAR];
        strcpy(lineOrig, line);

        removeComments(line);
        trim(line);
        trimEnd(line);
        trimBetween(line);

        if (*line == '\0')
            continue;

        paren_status result = parenthesis_check(line);

        if (result != PAREN_OK) {

            switch (result) {
                case PAREN_MISSING_CLOSE:
                    printf(""RC_FILE":%zu: expected ')'\n", lineC);
                    break;

                case PAREN_MISSING_OPEN:
                    printf(""RC_FILE":%zu: unexpected ')'\n", lineC);
                    break;

                case PAREN_UNCLOSED_QUOTE:
                    printf(""RC_FILE":%zu: unclosed quote\n", lineC);
                    break;

                default:
                    break;
            }

            goto fail;
        }

        char *args = NULL;
        char *cmd = extractCommandOrKey(line, &args);

        if (!cmd) continue;

        if (strcmp(cmd, "alias") == 0) {
            char *eq = findFirstEqualOutsideQuotes(lineOrig);
            if (!eq) {
                fprintf(stderr, ""RC_FILE":%zu: syntax error\n", lineC);
                goto fail;
            }

            *eq = '\0';
            char *shortcutName = strchr(lineOrig, ' ');
            char *action = eq + 1;

            trim(shortcutName); trimEnd(shortcutName);
            trim(action); trimEnd(action);

            if (!shortcutName || !*shortcutName) {
                fprintf(stderr, ""RC_FILE":%zu: missing shortcut name\n", lineC);
                goto fail;
            }

            if (!action || !*action) {
                fprintf(stderr, ""RC_FILE":%zu: missing action\n", lineC);
                goto fail;
            }

            if (!isBetweenQuotes(action, 2)) {
                fprintf(stderr, ""RC_FILE":%zu: the action should be between quotes\n", lineC);
                goto fail;
            }

        } else if (strcmp(cmd, "HISTSIZE") == 0) {
            char *val = getKeyVal("HISTSIZE", path);

            if (!val || !*val) {
                fprintf(stderr, ""RC_FILE":%zu: missing arguments!\n", lineC);
                goto fail;
            }

            float64 num = atof(val);

            if (isnan(num))
                goto fail;

            if (!isalldigit(val) || !T_CMP(num, (int64_t)num)) {
                fprintf(stderr, ""RC_FILE":%zu: arguments with invalid data type!\n", lineC);
                goto fail;
            }

            SAFE_FREE(val);

            if (num < HISTSIZE_MIN || num > HISTSIZE_MAX) {
                fprintf(stderr, ""RC_FILE":%zu: argument must be between 10 >= x <= 10000\n", lineC);
                goto fail;
            }

            if (isKeyRepeated(data_folder, "HISTSIZE")) {
                fprintf(stderr, ""RC_FILE":%zu: duplicate key found\n", lineC);
                goto fail;
            }

        } else {
            fprintf(stderr, ""RC_FILE":%zu: invalid key: '%s'\n", lineC, cmd);
            goto fail;
        }
    }

    SAFE_FREE(path);
    SAFE_FCLOSE(f);
    return;

fail:
    SAFE_FREE(path);
    SAFE_FCLOSE(f);
    exit(EXIT_FAILURE);
}


var calc(var left, const char *operation, var right, bool mathLib) {

    var out;
    out.type = BC_NONE;

    if (left.type == BC_NONE || right.type == BC_NONE) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("invalid data type: '"NONE_VAR"'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
        return out;
    }

    if (left.type == BC_STR || right.type == BC_STR) {

        if (strcmp(operation, "+") == 0) {

            if (left.type != right.type) {
                char type[0x20] = {0};
                var wrong = (left.type != BC_STR) ? left : right;

                getItemTypeStr(type, sizeof(type), wrong);

                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("cannot concatenate strings with '%s' type\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE, type);

                return out;
            }

            if (strlen(left.data.s) < 2 || strlen(right.data.s) < 2) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("invalid string format\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
                return out;
            }

            out.type = BC_STR;
            out.data.s = bc_strcat(left.data.s, right.data.s);

            if (!out.data.s) {
                out.type = BC_NONE;
                return out;
            }

            return out;
        } else if (strcmp(operation, "*") == 0) {

            var notStr;
            var Str;

            if (left.type != BC_STR) {
                notStr = left;
                Str = right;
            } else {
                notStr = right;
                Str = left;
            }

            if (notStr.type != BC_INT && notStr.type != BC_CHR) {
                char type[0x20] = {0};

                getItemTypeStr(type, sizeof(type), notStr);

                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("cannot multiply strings with type '%s'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE, type);

                return out;
            }

            int64_t multiplier = notStr.data.i; 
            char *multiplied_str = Str.data.s;

            if (multiplier <= 0) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("to multiply strings the multiplier must be at least greater than 0\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
                return out;
            }

            if (((ssize_t)strlen(multiplied_str) - 2) * (size_t)multiplier > MAX_CHAR) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("the resultant string must be less than %d characters long\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE, MAX_CHAR);
                return out;
            }

            out.type = BC_STR;
            char *result = strdup(multiplied_str);
            if (!result) {
                out.type = BC_NONE;
                return out;
            }

            for (size_t i = 1; i < (size_t)multiplier; i++) {
                char *old = result;
                result = bc_strcat(result, multiplied_str);

                SAFE_FREE(old);
                if (!result) { 
                    out.type = BC_NONE;
                    return out;
                }
            }

            out.data.s = result;
            return out;
        } else {

            if (strcmp(operation, "==") == 0) {
                out.type = BC_BOOL;

                if (left.type != right.type) {
                    out.data.b = false;
                    return out;
                }

                out.data.b = bc_strcmp(left.data.s, right.data.s) == 0;
                return out;
            } else if (strcmp(operation, "!=") == 0) {
                out.type = BC_BOOL;

                if (left.type != right.type) {
                    out.data.b = false;
                    return out;
                }

                out.data.b = bc_strcmp(left.data.s, right.data.s) != 0;
                return out;
            } else if (strcmp(operation, ">") == 0) {
                out.type = BC_BOOL;

                if (left.type != right.type) {
                    out.data.b = false;
                    return out;
                }

                out.data.b = bc_strcmp(left.data.s, right.data.s) > 0;
                return out;
            } else if (strcmp(operation, ">=") == 0) {
                out.type = BC_BOOL;

                if (left.type != right.type) {
                    out.data.b = false;
                    return out;
                }

                out.data.b = bc_strcmp(left.data.s, right.data.s) >= 0;
                return out;
            } else if (strcmp(operation, "<") == 0) {
                out.type = BC_BOOL;

                if (left.type != right.type) {
                    out.data.b = false;
                    return out;
                }

                out.data.b = bc_strcmp(left.data.s, right.data.s) < 0;
                return out;
            } else if (strcmp(operation, "<=") == 0) {
                out.type = BC_BOOL;

                if (left.type != right.type) {
                    out.data.b = false;
                    return out;
                }

                out.data.b = bc_strcmp(left.data.s, right.data.s) <= 0;
                return out;
            } else {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("unsupported operand for 'str' type: '%s'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE, operation);
                return out;
            }
        }
    }

    float64 num1;
    float64 num2;

    switch (left.type) {
        case BC_CHR:
        case BC_BOOL:
        case BC_INT:
            num1 = (double)left.data.i;
            break;
        case BC_FLOAT:
            num1 = left.data.f;
            break;
        default:
            return out;
    }

    switch (right.type) {
        case BC_CHR:
        case BC_BOOL:
        case BC_INT:
            num2 = (double)right.data.i;
            break;
        case BC_FLOAT:
            num2 = right.data.f;
            break;
        default:
            return out;
    }

    float64 result = 0;

    if (strcmp(operation, "+") == 0)
        result = num1 + num2;

    else if (strcmp(operation, "-") == 0)
        result = num1 - num2;

    else if (strcmp(operation, "*") == 0)
        result = num1 * num2;

    else if (strcmp(operation, "/") == 0) {

        if (num2 == 0.0) {

            if (!mathLib) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("cannot divide by 0\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
                return out;
            }

            int32_t negative = signbit(num1) ^ signbit(num2);
            result = negative ? -INFINITY : INFINITY;

        } else
            result = num1 / num2;
    }

    else if (strcmp(operation, "%") == 0) {

        if (num2 == 0) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("cannot divide by 0\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        }

        result = fmod(num1, num2);
    }

    else if (strcmp(operation, "^") == 0) {

        if ((left.type != BC_INT && left.type != BC_CHR) || (right.type != BC_INT && right.type != BC_CHR)) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("'^' requires type '"INT_VAR"'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        }

        result = (int64_t)num1 ^ (int64_t)num2;
    }

    else if (strcmp(operation, "&") == 0) {

        if ((left.type != BC_INT && left.type != BC_CHR) || (right.type != BC_INT && right.type != BC_CHR)) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("'&' requires type '"INT_VAR"'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        }

        result = (int64_t)num1 & (int64_t)num2;
    }

    else if (strcmp(operation, "|") == 0) {

        if ((left.type != BC_INT && left.type != BC_CHR) || (right.type != BC_INT && right.type != BC_CHR)) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("'|' requires type '"INT_VAR"'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        }

        result = (int64_t)num1 | (int64_t)num2;
    }

    else if (strcmp(operation, "<") == 0) {

        out.type = BC_BOOL;
        out.data.b = (num1 < num2) && !T_CMP(num1, num2);
        return out;
    }

    else if (strcmp(operation, ">") == 0) {

        out.type = BC_BOOL;
        out.data.b = (num1 > num2) && !T_CMP(num1, num2);
        return out;
    }

    else if (strcmp(operation, "<=") == 0) {

        out.type = BC_BOOL;
        out.data.b = (num1 < num2) || T_CMP(num1, num2);
        return out;
    }

    else if (strcmp(operation, ">=") == 0) {

        out.type = BC_BOOL;
        out.data.b = (num1 > num2) || T_CMP(num1, num2);
        return out;
    }

    else if (strcmp(operation, "!=") == 0) {

        out.type = BC_BOOL;
        out.data.b = T_CMP(num1, num2);
        return out;
    }

    else if (strcmp(operation, "==") == 0) {

        out.type = BC_BOOL;

        if (isnan(num1) || isnan(num2))
            out.data.b = false;

        else if (isinf(num1) || isinf(num2))
            out.data.b = (num1 == num2);

        else
            out.data.b = T_CMP(num1, num2);

        return out;
    }

    else if (strcmp(operation, "**") == 0) {

        if (num1 < 0 && right.type == BC_INT) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("negative base with non-integer exponent\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        }

        result = pow(num1, num2);
    }


    else if (strcmp(operation, "^^") == 0) {

        if (right.type != BC_INT) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("tetration height must be of type '"INT_VAR"'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        } else if (num2 < 0) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("tetration height must be non-negative\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        } else if (num1 == 0.0 && num2 == 0.0) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("0^^0 is undefined\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);            
            return out;
        } else {

            result = tetration(num1, (int32_t)num2);

            if (isnan(result)) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("invalid input for tetration\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            }
        }
    }

    else if (strcmp(operation, "<<") == 0) {

        if (num2 < 0 || num2 >= sizeof(uint64_t) * 8) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("invalid shift\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        }

        result = (uint64_t)num1 << (uint64_t)num2;
    }

    else if (strcmp(operation, ">>") == 0) {

        if (num2 < 0 || num2 >= sizeof(int64_t) * 8) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("invalid shift\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
            return out;
        }

        result = (int64_t)num1 >> (int64_t)num2;
    }

    else if (strcmp(operation, "&&") == 0) {

        out.type = BC_BOOL;
        out.data.b = (num1 != 0 && num2 != 0);
        return out;
    }

    else if (strcmp(operation, "||") == 0) {

        out.type = BC_BOOL;
        out.data.b = (num1 != 0 || num2 != 0);
        return out;
    }

    else {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("Unknown operator '%s'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE, operation);
        return out;
    }

    if (T_CMP(result, (int64_t)result)) {

        out.type = BC_INT;
        out.data.i = (int64_t)result;

    } else {

        out.type = BC_FLOAT;
        out.data.f = result;
    }

    return out;
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

        printf("\nEnvironment variables recognized: (not case sensitive)\n");
        printf("\t$PATH                     <-- system PATH\n");
        printf("\t$HOME                     <-- home directory (Linux)\n");
        printf("\t$USERNAME / $USER         <-- username (Windows/Linux)\n");
        printf("\t$TEMP                     <-- temporary folder\n");

    #ifndef _WIN64
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
        );
        
        printf("\nEnvironment variables recognized: (not case sensitive)\n");
        printf("\t$PATH                     <-- system PATH\n");
        printf("\t$HOME                     <-- home directory (Linux)\n");
        printf("\t$USERNAME / $USER         <-- username (Windows/Linux)\n");
        printf("\t$TEMP                     <-- temporary folder\n");

    #ifndef _WIN64
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
        printf("'bc' a simple calculator on the terminal, operand precedence does not work unless if you use parenthesis, and it also works with strings\n");
        printf("\nUsage:\n\tbc [OPTION...]\n\nOptions:\n");
        printf("\t'-q', '--quiet'     will not print the initial text\n");
        printf("\t'-l', '--mathlib'   includes the mathlib header\n");
        printf("\nOperations:\n"
            "\t'+'    : Addition / string concatenation\n"
            "\t         Example: 2 + 3 = 5 / \"string1\" + \"string2\" = \"string1string2\"\n"
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
            "\t         Explanation: 0b01000 >> 2 = 0b00010, the bits were dislocated 2 times to the right\n"  
            "\n"
            "\t'!'    : Logical NOT\n"
            "\t         Example: !true = false / !false = true\n"
            "\t         Explanation: the inverse of true is false and the inverse of false is true\n"
            "\n"
            "\t'&&'   : Logical AND\n"
            "\t         Example: 5 && 0 = false\n"
            "\t         Note: Any non-zero value is treated as true\n"
            "\t         Explanation: Result is true(1) only if both operands are true(1), otherwise returns false(0)\n"
            "\n"
            "\t'||'   : Logical OR\n"
            "\t         Example: 0 || 5 = true\n"
            "\t         Explanation: Result is true(1) if at least one operand is true(1), otherwise returns false(0)\n"
            "\n"
            "\t'<'    : Less than\n"
            "\t         Example: 3 < 5 = true / \"apple\" < \"banana\" = true\n"
            "\t         Explanation: returns true(1) if the first value is less than the second, otherwise returns false(0)\n"
            "\n"
            "\t'<='   : Less than or equal\n"
            "\t         Example: 5 <= 5 = true / \"apple\" <= \"apple\" = true\n"
            "\t         Explanation: returns true(1) if the first value is less or equal than the second, otherwise returns false(0)\n"
            "\n"
            "\t'>'    : Greater than\n"
            "\t         Example: 8 > 3 = true / \"banana\" > \"apple\" = true\n"
            "\t         Explanation: returns true(1) if the first value is greater than the second, otherwise returns false(0)\n"
            "\n"
            "\t'>='   : Greater than or equal\n"
            "\t         Example: 4 >= 4 = true / \"banana\" >= \"banana\" = true\n"
            "\t         Explanation: returns true(1) if the first value is greater or equal than the second, otherwise returns false(0)\n"
            "\n"
            "\t'=='   : Equal to\n"
            "\t         Example: 6 == 6 = true / \"same\" == \"same\" = true\n"
            "\t         Explanation: returns true(1) if the first value is equal to the second, otherwise returns false(0)\n"
            "\n"
            "\t'!='   : Not equal to\n"
            "\t         Example: 6 != 5 = true / \"apple\" != \"banana\" = true\n"
            "\t         Explanation: returns true(1) if the first value is different from the second, otherwise returns false(0)\n"


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
        );
        printf(       
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
            "\t                 Note: Returns value in radians (range: 0 < result < PI)\n"
            "\n"
            "\tdeg2rad(X)     : Degrees to radians\n"
            "\t                 Example: rad(3.1415) = 180\n"
            "\n"
            "\trad2deg(X)     : Radians to degrees\n"
            "\t                 Example: deg(180) = 3.1415\n"
            "\n"
            "\trad2gon(X)     : Radians to gradians\n"
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
            "\t                 Example: hex(255) = "HEX_PREF"0FF\n"
            "\n"
            "\toct(X)         : Convert X to octal\n"
            "\t                 Example: oct(8) = "OCT_PREF"10\n"
            "\n"
            "\tbin(X)         : Convert X to binary\n"
            "\t                 Example: bin(5) = "BIN_PREF"0101\n"
        );
        printf(
            "\n"
            "\tchr(X)         : Convert X to ascii\n"
            "\t                 Example: ascii(65) = 'A'\n"
            "\t                 Note: escape characters does not work\n"
            "\t                 Tip: requires an argument of type '"INT_VAR"' between 0 and 127 (inclusive)\n"
            "\n"
            "\t"INT_VAR"(X)         : Converts X to integer\n"
            "\t                 Example: int(\"2\") = 2\n"
            "\n"
            "\t"FLOAT_VAR"(X)       : Converts X to float\n"
            "\t                 Example: float(\"3.1415\") = 3.1415\n"
            "\n"
            "\t"STR_VAR"(X)         : Converts X to string\n"
            "\t                 Example: str(PI) = \"3.14159\"\n"
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
            "\tabs(X)         : Absolute value (int and float values)\n"
            "\t                 Example: abs(-7) = 7\n"
            "\n"
            "\tlen(str)       : Returns the length of a string\n"
            "\t                 Example: strlen(\"string\") = 6\n"
            "\n"
            "\tbmi(X, Y)      : Returns your BMI with wight (X) in kg and height (Y) in meters\n"
            "\t                 Example: bmi(91, 1.78) = 28.7211\n"
            "\n"
            "\tisprime(X)     : Returns 1 if X is prime, otherwise it returns 0\n"
            "\t                 Example: isprime(5) = 1\n"
            "\t                 Note: it requires an argument of type '"INT_VAR"' which is greater 1\n"
            "\n"
            "\tlower(X)       : Returns the string in lower case form\n"
            "\t                 Example: lower(\"STRING\") = \"string\"\n"
            "\n"
            "\tupper(X)       : Returns the string in upper case form\n"
            "\t                 Example: lower(\"string\") = \"STRING\"\n"
            "\n"
            "\ttypeof(X)      : Returns the type of the argument as a string\n"
            "\t                 Example: typeof(\"string\") = \""STR_VAR"\" / typeof(2) = \""INT_VAR"\"\n"

            "\nBuiltin Variables: (mathlib must be on to grant access)\n"
            "\tAns   : stores the result of the last operation\n"
            "\t        Tip: initially it is undefined\n"
            "\t        Note: its value cannot be changed manually\n"

            "\nConstants: (mathlib must be on to grant access)\n"
            "\tPI      : 3.141592...\n"
            "\t          Example: sin(PI / 2) = 1\n"
            "\n"
            "\tE       : 2.718281...\n"
            "\t          Example: ln(E) = 1\n"
            "\n"
            "\tINF     : 1.797e+308 (64 bit)\n"
            "\t          Example: acot(-inf) = PI\n"
            "\n"
            "\ttrue    : 1 (boolean)\n"
            "\t         Example: sen(deg2rad(30)) == sen(deg2rad(150))\n"
            "\n"
            "\tfalse   : 0 (boolean)\n"
            "\t          Example: 1 != 5 = false\n"
            "\n"

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
            "\n"
            "\t'!' : factorial\n"
            "\t      Example: 5! = 120 / 5!! = 15\n"

            "\nNumeric systems: (mathlib must be on to grant full access)\n"
            "\tBinary: (prefix: '"BIN_PREF"')        base 2 numbers e.g. "BIN_PREF"010000000000 = 1024\n"
            "\n"
            "\tDecimal: (default):           base 10 numbers e.g. 1024\n"
            "\n"
            "\tOctal: (prefix: '"OCT_PREF"')         base 8 numbers e.g. "OCT_PREF"2000 = 1024\n"
            "\n"
            "\tHexadecimal: (prefix: '"HEX_PREF"')   base 16 numbers e.g. "HEX_PREF"400 = 1024\n"
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
        printf("'sleep' delay for a specified amount of time\n\nUsage:\n\tsleep [float: TIME]\n\nSuffixes: (not case sensitive)\n");
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

void processCommand(char *input, const char **cmds, char **address, char *history_path,
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
            trimBetween(segment);

            if (*segment) {
                processCommand(segment, cmds, address, history_path,
                                data_folder, isInsideBash, false);
            }

            if (!pos)
                break;
        }

        SAFE_FREE(temp);
        return;
    }


    #ifdef _WIN64
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

    char *args;
    char *instruction = extract_instruction(temp, &args);

    if (!instruction) {
        SAFE_FREE(temp);
        return;
    }

    trim(instruction);
    trimEnd(instruction);
    trimBetween(instruction);

    if (!isFromAlias && isalias(instruction, args ? args : "", cmds, address, 
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
        SAFE_FREE(path);
    }

    else if (strcmp(instruction, cmds[4]) == 0) //! updatehistory
        updatehistory();

    else if (strcmp(instruction, cmds[5]) == 0) //! cmds
        cmdsCommand(cmds, isInsideBash);

    else if (strcmp(instruction, cmds[6]) == 0) { //! cd
    #ifdef _WIN64           
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

    else if (strcmp(instruction, cmds[13]) == 0) { //! rmdir
        uint16_t argc_rmdir;
        char **argv_rmdir = extract_args(args, &argc_rmdir, "rmdir");

        rmdirCmd(args ? argc_rmdir : 1, argv_rmdir);

        if (args) {
            for (uint16_t i = 0; i < argc_rmdir; i++)
                SAFE_FREE(argv_rmdir[i]);
            SAFE_FREE(argv_rmdir);
        }
    }

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

        bcCmd(argc_bc, argv_bc);
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

        bashCmd(argc_bash, argv_bash, cmds, true);

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

var parse_operation(char *operation, const FuncEntry *functions, size_t funcCount, const char *uniOps, const char **multiOps, bool mathlib) {
    char op[0x4] = {0};

    while (is_wrapped_by_parentheses(operation)) {
        size_t len = strlen(operation);
        if (len <= 2)
            break;

        operation[len - 1] = '\0';
        memmove(operation, operation + 1, len);

        trim(operation);
        trimEnd(operation);
    }

    int16_t op_pos = find_main_operator_full(operation, multiOps, uniOps, op);

    if (op_pos == -1) {

        if (mathlib && Ans.type == BC_STR && strcmp(operation, ANS_VAR) == 0)
            return (var){ .type = BC_STR, .data.s = strdup(Ans.data.s)};

        if (*operation == '!') {

            size_t count = 0;
            while (operation[count] == '!')
                count++;

            char *expr = operation + count;

            if (!*expr) {
                printc("eval", BC_PROMPT_COLOR, WHITE);
                printf(": ");
                printc("syntax error\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);

                return (var){ .type = BC_NONE };
            }

            if (isBetweenQuotes(expr, 1)) {
                if (count & 1)
                    return (var){ .type = BC_BOOL, .data.b = false };
                else
                    return (var){ .type = BC_BOOL, .data.b = true };
            }

            if (mathlib) {
                if (Ans.type == BC_STR && strcmp(expr, ANS_VAR) == 0) {
                    if (!Ans.data.s)
                        return (var){ .type = BC_BOOL, .data.b = false };

                    if (count & 1)
                        return (var){ .type = BC_BOOL, .data.b = false };
                    else
                        return (var){ .type = BC_BOOL, .data.b = true };
                }
            }

            char *buff = eval(expr, mathlib);
            if (!buff)
                return (var){ .type = BC_NONE };

            var tmp = h_atof(buff, mathlib);
            SAFE_FREE(buff);

            float64 num = (tmp.type == BC_BOOL) ? (float64)tmp.data.b : tmp.data.f;
            if (tmp.type == BC_INT) {
                int64_t val = tmp.data.i;
                bool value = (val != 0.0);

                if (count & 1)
                    value = !value;

                return (var){ .type = BC_BOOL, .data.b = value };
            }

            if (isnan(num))
                return (var){ .type = BC_NONE };

            bool value = (num != 0.0);

            if (count & 1)
                value = !value;

            return (var){ .type = BC_BOOL, .data.b = value };
        }

        char *paren = strchr(operation, '(');

        if (!paren) {

            if (strcmp(operation, TRUE_VAR) == 0)
                return (var){ .type = BC_BOOL, .data.b = true };
            else if (strcmp(operation, FALSE_VAR) == 0)
                return (var){ .type = BC_BOOL, .data.b = false };

            size_t len = strlen(operation);

            if (len > 1 && operation[0] == '"') {

                char result[0x400] = {0};
                size_t res_len = 0;

                const char *p = operation;

                while (*p) {

                    while (isspace((unsigned char)*p))
                        p++;

                    if (*p != '"')
                        break;

                    p++;

                    while (*p) {

                        if (*p == '"') {

                            uint16_t backslashes = 0;
                            const char *q = p - 1;

                            while (q >= operation && *q == '\\') {
                                backslashes++;
                                q--;
                            }

                            if ((backslashes & 1) == 0)
                                break;
                        }

                        result[res_len++] = *p;
                        p++;
                    }

                    if (*p != '"') {
                        printc("eval", BC_PROMPT_COLOR, WHITE);
                        printf(": ");
                        printc("unclosed quote\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);

                        return (var){ .type = BC_NONE };
                    }

                    p++;
                }

                if (res_len > 0) {

                    char *final = malloc(res_len + 3);
                    if (!final) {
                        printc("eval", BC_PROMPT_COLOR, WHITE);
                        printf(": ");
                        printc("memory allocation error\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);
                        return (var){ .type = BC_NONE };
                    }

                    final[0] = '"';
                    memcpy(final + 1, result, res_len);
                    final[res_len + 1] = '"';
                    final[res_len + 2] = '\0';

                    return (var){ .type = BC_STR, .data.s = final };
                }
            }

            var tmp = h_atof(operation, mathlib);

            if (tmp.type == BC_FLOAT && isnan(tmp.data.f))
                return (var){ .type = BC_NONE };

            return tmp;
        }

        if (operation[strlen(operation)-1] == '!') {
            int64_t result = s_fact(operation);

            if (result == I64_NAN)
                return (var){ .type = BC_NONE };

            return (var){ .type = BC_INT, .data.i = result };
        }

        char name[0x100] = {0};

        ssize_t parenthesis_index = strchar(operation, '(');
        if (parenthesis_index == -1)
            return h_atof(operation, mathlib);

        memcpy(name, operation, parenthesis_index);
        name[parenthesis_index] = '\0';

        trimEnd(name);

        if (!*name)
            return (var){ .type = BC_NONE };

        if (*operation == '~' || *operation == '-') {
            var tmp = h_atof(operation, mathlib);

            if (tmp.type == BC_FLOAT && isnan(tmp.data.f))
                return (var){ .type = BC_NONE };

            return tmp;
        }

        int32_t depth = 0;
        int32_t close_index = -1;

        for (int32_t i = parenthesis_index; operation[i]; i++) {
            if (operation[i] == '(')
                depth++;
            else if (operation[i] == ')') {
                depth--;
                if (depth == 0) {
                    close_index = i;
                    break;
                }
            }
        }

        if (close_index == -1 || operation[close_index + 1] != '\0') {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("invalid syntax\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);

            return (var){ .type = BC_NONE };
        }

        if (!isValidBcFuncName(name)) {
            printc("eval", BC_PROMPT_COLOR, WHITE);
            printf(": ");
            printc("invalid function name: '%s()'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE, name);

            return (var){ .type = BC_NONE };
        }

        if (mathlib) {
            for (size_t i = 0; i < funcCount; i++) {
                if (strcmp(name, functions[i].name) != 0)
                    continue;

                switch (functions[i].returnType) {
                    case BC_BOOL:
                    case BC_INT: {
                        int64_t num = functions[i].fn.i(operation);

                        if (num == I64_NAN)
                            return (var){ .type = BC_NONE };

                        if (functions[i].returnType == BC_BOOL)
                            return (var){ .type = BC_BOOL, .data.b = (bool)num };

                        return (var){ .type = functions[i].returnType, .data.i = num };
                    }

                    case BC_FLOAT: {
                        float64 num = functions[i].fn.f(operation);

                        if (isnan(num))
                            return (var){ .type = BC_NONE };

                        return (var){ .type = functions[i].returnType, .data.f = num };
                    }

                    case BC_STR: {
                        char *result = functions[i].fn.s(operation);

                        if (!result)
                            return (var){ .type = BC_NONE };

                        return (var){ .type = functions[i].returnType, .data.s = result};
                    }

                    default: {
                        char type[0x14] = {0};

                        getItemTypeStr(type, sizeof(type), (var){.type = functions[i].returnType});

                        printc("eval", BC_PROMPT_COLOR, WHITE);
                        printf(": ");
                        printc("invalid function with '%s' unknown type: '%s'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE, type, name);

                        return (var){ .type = BC_NONE} ;
                    }
                }
            }
        }

        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("undefined function: '%s()'\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE, name);

        return (var){ .type = BC_NONE };
    }

    char buffer[0x100];
    strncpy(buffer, operation, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    buffer[op_pos] = '\0';

    char *num1 = buffer;
    char *num2 = buffer + op_pos + strlen(op);

    trim(num1); trimEnd(num1);
    trim(num2); trimEnd(num2);

    if (!*num1 || !*num2) {
        printc("eval", BC_PROMPT_COLOR, WHITE);
        printf(": ");
        printc("invalid syntax\n", GET_BASE_COLOR(BC_PROMPT_COLOR), WHITE);

        return (var){ .type = BC_NONE };
    }

    char *left = eval(num1, mathlib);
    if (!left)
        return (var){ .type = BC_NONE };

    char *right = eval(num2, mathlib);
    if (!right) {
        SAFE_FREE(left);
        return (var){ .type = BC_NONE };
    }

    var val1 = parse_operation(left, functions, funcCount, uniOps, multiOps, mathlib);
    var val2 = parse_operation(right, functions, funcCount, uniOps, multiOps, mathlib);

    if (val1.type == BC_NONE || val2.type == BC_NONE)
        return (var){ .type = BC_NONE };

    var result = calc(val1, op, val2, mathlib);

    SAFE_FREE(left);
    SAFE_FREE(right);

    return result;
}
