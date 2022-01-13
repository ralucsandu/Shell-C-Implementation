#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define MAX_PATH 1000

#define MAX_MATRIX 1000 

char ** history_command;
int history_index;


size_t COMMAND_MAX = 1000;
int COMMAND_MAX_WORDS = 100;

char cwd[MAX_PATH];

char* command;

char** parsed_command;
int parsed_command_size;

void welcomeShell()
{
    printf("\n\n\n******************"
        "************************");
    char* username = getenv("USER");
    printf("\n\n\tWelcome to shell @%s", username);
    printf("\n\n*******************"
        "***********************");
    printf("\n\n");
}

/**
 * Takes the command from `command`, and parses it into
 * independent words.
 * 
 * return -1 if the command is not valid.
 * 
 * RULES:
 *  * ', " and ` will be treated as special characters
 *  * ', ", ` and \ have to be manually escaped with a \
 */

int ParseCommand()
{
    int is_in_quotes = 0;
    int is_escaped = 0;
    
    parsed_command_size = 0;

    char* cnt_word = malloc(COMMAND_MAX);
    int cnt_word_size = 0;

    for (int poz_act = 0; poz_act < strlen(command); poz_act++) {
        char act = command[poz_act];

        // caz 1 -> se sparge un cuvant
        if (!is_escaped && !is_in_quotes && (act == ' ' || act == '\n' || act == '\t')) {
            
            if (cnt_word_size != 0) {
                // adaugam null ca sa stie strcpy unde sa se opreasca
                cnt_word[cnt_word_size] = 0;
                strcpy(parsed_command[parsed_command_size], cnt_word);
                parsed_command_size++;
                cnt_word_size = 0;
            }
        }
        // caz 2 -> citim un \ care da escape
        else if (!is_escaped && act == '\\')
            is_escaped = 1;
        // caz 3 -> citim un " care nu este escape-uit
        else if (!is_escaped && (act == '\'' || act == '\"' || act == '`'))
            is_in_quotes = 1 - is_in_quotes;
       
        else {
            cnt_word[cnt_word_size] = act;
            cnt_word_size++;
            is_escaped = 0;
        }
    }

    if (cnt_word_size != 0 || is_escaped || is_in_quotes)
        return -1;
    return 0;
}

/**
 * Changes the directory of our process
 */
 
int ChangeDirectory()
{
    if (parsed_command_size != 2) {
        printf("Invalid command.\nUsage of cd:\n  cd new-folder\n");
        return 0;
    }
    int chdir_result = chdir(parsed_command[1]);
    if (chdir_result != 0)
        printf("The selected directory is not valid!\n");
    return 0;
}

void AddHistory(char* add_history_line)
{
    if (history_index == 0)
        {
            history_command = malloc((MAX_MATRIX+1) * sizeof(*history_command));

            for (int i = 0; i < MAX_MATRIX; i++) 
                history_command[i] = malloc(COMMAND_MAX);
                
        }
    strcpy(history_command[history_index], add_history_line);
    history_index +=1;

}
/**
 * Prints the history commands
 */
int History()
{
    for(int i=0; i<history_index; i++)
        printf("%d %s\n", i+1, history_command[i]);
    return 0;
}

/**
 * Runs a custom program.
 */
int CustomCommand(char** parsed_command_act, int parsed_command_size_act, const char* new_stdin_file, const char* new_stdout_file)
{
    char* real_program = NULL;
    
    // caz 1 -> comanda incepe cu un . sau /
    if ((parsed_command_act[0][0] == '.' && parsed_command_act[0][1] == '/') || parsed_command_act[0][0] == '/') {
        real_program = realpath(parsed_command_act[0], NULL);
        if (real_program == NULL)
            return -1;
        
    }
    // caz 2 -> comanda este data fara inceput specific -> trebuie sa o cautam in PATH
    else {
        real_program = malloc(strlen(parsed_command_act[0] + 1));
        strcpy(real_program, parsed_command_act[0]);
    }

    if (real_program == NULL) {
        printf("The specified program was not found!\n");
        return 0;
    }

    int pid = fork();
    if (pid == 0) {
        if (new_stdin_file)
            freopen(new_stdin_file, "r", stdin);
        if (new_stdout_file)
            freopen(new_stdout_file, "w", stdout);

        char** args = malloc((parsed_command_size_act + 1) * sizeof(*args));
        for (int i = 0; i < parsed_command_size_act; i++) {
            args[i] = malloc(strlen(parsed_command_act[i]) + 1);
            strcpy(args[i], parsed_command_act[i]);
        }
        args[parsed_command_size_act] = NULL;
        
        execvp(real_program, args);
    }
    else {
        wait(NULL);
    }

    free(real_program);
    return 0;
}

/**
 * Returns -1 if the command is not valid
 */
int ProcessCommand()
{
    int parse_command = ParseCommand();
    if (parse_command == -1)
        return -1;
    
    if (parsed_command_size == 0)
        return 0;
      
    
    if (strcmp("history", parsed_command[0]) == 0)
        return History();
    
    
    if (strcmp("quit", parsed_command[0]) == 0)
        exit(0);
    
    int indice, ok =0; 
   
    for (indice = 0; indice < parsed_command_size - 1 && ok == 0; indice++)
      {

        if(strcmp("&&", parsed_command[indice]) == 0 || strcmp("||", parsed_command[indice]) == 0)
          {
            ok = 1;
          }
      }

    indice--;   
    if(ok == 1)
    {  
        int rezultat = 2;
        if (indice == 1){
            if (strcmp(parsed_command[0],"true") == 0 )
                rezultat = 0;
            else 
            if (strcmp(parsed_command[0],"false") == 0 )
                 rezultat = -1;
           
          }
      
        if (rezultat == 2)
        { 
            rezultat = CustomCommand(parsed_command,
                          indice,
                          NULL,
                          NULL
                          );
        }

        if( (rezultat == 0 && strcmp("&&", parsed_command[indice]) == 0)
            ||  (rezultat != 0 && strcmp("||", parsed_command[indice]) == 0))
        {
        rezultat = 2;
        if (parsed_command_size - indice -1 == 1)
            if (strcmp(parsed_command[indice + 1],"true") == 0 )
                rezultat = 0;
            else 
             if (strcmp(parsed_command[indice + 1],"false") == 0 )
                 rezultat = -1;
        }
    
    if(rezultat == 2)
        CustomCommand(parsed_command + indice +1,
                      parsed_command_size - indice -1,
                      NULL,
                      NULL
                      );
    }
    else
    {

      // 1st case: system command
      if (strcmp("cd", parsed_command[0]) == 0)
          return ChangeDirectory();

    // 2nd case: custom user command
    int begin_command = 0;
    
    char* pipe_names[] = { "pipe1", "pipe2" };
    int cnt_pipes = 0;

    for (int poz_command = 0; poz_command <= parsed_command_size; poz_command++) {
        if (strcmp(parsed_command[poz_command], "|") == 0 || poz_command == parsed_command_size) {
            int result = CustomCommand(
                parsed_command + begin_command,
                poz_command - begin_command,
                (begin_command == 0 ? NULL : pipe_names[cnt_pipes]),
                (poz_command == parsed_command_size ? NULL : pipe_names[1 - cnt_pipes])
            );
            cnt_pipes = 1 - cnt_pipes;
            begin_command = poz_command + 1;

            if (result != 0)
                return result;
        }
    }
    }
    return 0;
}

int main()
{
    welcomeShell();
    command = malloc(COMMAND_MAX);
    parsed_command = malloc(COMMAND_MAX_WORDS * sizeof(*parsed_command));
    for (int i = 0; i < COMMAND_MAX_WORDS; i++)
        parsed_command[i] = malloc(COMMAND_MAX * sizeof(1));

    while (1) {
        getcwd(cwd, sizeof(cwd));
        printf("%s$ ", cwd);

        size_t n = COMMAND_MAX;
        getline(&command, &n, stdin);

        AddHistory(command); 

        int process_command = ProcessCommand();

        if (process_command == -1) {
            command[strlen(command) - 1] = 0;
            printf("The command \"%s\" is not valid.\n", command);
        }
    }
    return 0;
}