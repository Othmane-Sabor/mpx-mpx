//
// This file contains all the commands and functions related to the command handler
//

//
// This file contains all the commands and functions related to the command handler
//

#include <mpx/io.h>
#include <sys_req.h>
#include <string.h>
#include <stdlib.h>
#include <pcb.h>

// RTC Register addresses (from the Intel document)
#define RTC_INDEX_PORT 0x70
#define RTC_DATA_PORT 0x71

// RTC Register indices for Date and Time (from the Intel document)
#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY_OF_MONTH 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09

// Function to read a byte from RTC (using Intel document references)
unsigned char read_rtc(unsigned char reg)
{
    outb(RTC_INDEX_PORT, reg);
    return inb(RTC_DATA_PORT);
}

// Function to write a byte to RTC (using Intel document references)
void write_rtc(unsigned char reg, unsigned char value)
{
    outb(RTC_INDEX_PORT, reg);
    outb(RTC_DATA_PORT, value);
}

// BCD to binary conversion
unsigned char bcd_to_binary(unsigned char bcd)
{
    return (bcd & 0x0F) + ((bcd >> 4) * 10);
}

// Binary to BCD conversion
unsigned char binary_to_bcd(unsigned char binary)
{
    return ((binary / 10) << 4) | (binary % 10);
}

// Function to convert binary to ASCII representation
void binary_to_ascii(unsigned char value, char *buffer)
{
    buffer[0] = (value / 10) + '0';
    buffer[1] = (value % 10) + '0';
    buffer[2] = '\0';
}

// Function to get the current date from RTC
void get_date_command()
{
    unsigned char day, month, year;
    day = bcd_to_binary(read_rtc(RTC_DAY_OF_MONTH));
    month = bcd_to_binary(read_rtc(RTC_MONTH));
    year = bcd_to_binary(read_rtc(RTC_YEAR));

    char date_str[12];
    binary_to_ascii(month, date_str);
    date_str[2] = '/';
    binary_to_ascii(day, date_str + 3);
    date_str[5] = '/';
    binary_to_ascii(year, date_str + 6);
    date_str[8] = '\r';
    date_str[9] = '\n';
    date_str[10] = '\0';

    sys_req(WRITE, COM1, date_str, strlen(date_str));
}

// Function to get the current time from RTC
void get_time_command()
{
    unsigned char hours, minutes, seconds;
    seconds = bcd_to_binary(read_rtc(RTC_SECONDS));
    minutes = bcd_to_binary(read_rtc(RTC_MINUTES));
    hours = bcd_to_binary(read_rtc(RTC_HOURS));

    char time_str[12];
    binary_to_ascii(hours, time_str);
    time_str[2] = ':';
    binary_to_ascii(minutes, time_str + 3);
    time_str[5] = ':';
    binary_to_ascii(seconds, time_str + 6);
    time_str[8] = '\r';
    time_str[9] = '\n';
    time_str[10] = '\0';

    sys_req(WRITE, COM1, time_str, strlen(time_str));
}

// shutdown flag
// shutdown flag
int should_shutdown = 0;

typedef struct
{
    const char *name;
    void (*handler)(const char *args);
    const char *description;
} command_t;

// Forward declarations
void version_command();
void help_command();
void shutdown_command();
void get_date_command();
void get_date_command();
void set_date_command(const char *args);
void get_time_command();
void get_time_command();
void set_time_command(const char *args);

// PCB command declarations
void create_pcb_command(const char *args);
void show_pcb_command(const char *args);
void show_all_pcbs_command();
void show_ready_pcbs_command();
void show_blocked_pcbs_command();
void delete_pcb_command(const char *name);
void suspend_pcb_command(const char *name);
void resume_pcb_command(const char *name);
void block_pcb_command(const char *name);
void unblock_pcb_command(const char *name);
void set_pcb_priority_command(const char *args);
void yield_command(const char *args);
void loadR3_command(const char *args);

//com struct
command_t commands[] = {
    {"version", version_command, "Displays the current version of MPX and the compilation date"},
    {"shutdown", shutdown_command, "Shutdown the system with confirmation"},
    {"help", help_command, "Provides usage instruction for all commands"},
    {"getdate", get_date_command, "Get the current date"},
    {"setdate", set_date_command, "Set the date: 'setdate [MM/DD/YY]'"},
    {"gettime", get_time_command, "Get the current time"},
    {"settime", set_time_command, "Set the time: 'settime [hh:mm:ss]'"},
    {"createpcb", create_pcb_command, "Creates a new PCB: createpcb [name] [class ('user' or 'system')] [priority (0-9)]"},
    {"showpcb", show_pcb_command, "Shows a given PCB if it exists: showpcb [name]"},
    {"showreadypcbs", show_ready_pcbs_command, "Shows all existing PCBs in the Ready state"},
    {"showblockedpcbs", show_blocked_pcbs_command, "Shows all existing PCBs in the Blocked state"},
    {"showallpcbs", show_all_pcbs_command, "Shows all existing PCBs"},
    {"deletepcb", delete_pcb_command, "Deletes a PCB by name: 'deletepcb [name]'"},
    {"suspendpcb", suspend_pcb_command, "Suspend a PCB by name: 'suspendpcb [name]'"},
    {"resumepcb", resume_pcb_command, "Resume a suspended PCB by name: 'resumepcb [name]'"},
    {"blockpcb", block_pcb_command, "Block a PCB by name: 'blockpcb [name]'"},
    {"unblockpcb", unblock_pcb_command, "Unblock a PCB by name: 'unblockpcb [name]'"},
    {"setpcbprio", set_pcb_priority_command, "Sets the priority of a PCB: 'setpcbprio [name] [newpriority (0-9)]'"},
    {"yield",yield_command,"Yield the CPU"},
    {"loadR3",loadR3_command,"Load R3"},
    {NULL, NULL, NULL}};

// Function to remove trailing whitespace from input
void trim_input(char *buf)
{
    char *end = buf + strlen(buf) - 1;
    while (end > buf && (*end == ' ' || *end == '\n' || *end == '\r'))
    {
        *end = 0;
        end--;
    }
}

// Command for displaying the current version and most recent compilation date
void version_command(const char *args)
{
    (void)args; // Mark the parameter as unused

    char version_msg[] = "MPX Version: R2, Compiled on: 3rd October 2023\r\n";
    sys_req(WRITE, COM1, version_msg, sizeof(version_msg));
}

// Command for displaying usage instructions for all commands
void help_command(const char *args)
{
    if (args)
    {
        for (command_t *cmd = commands; cmd->name; cmd++)
        {
            if (strcmp(args, cmd->name) == 0)
            {
                sys_req(WRITE, COM1, cmd->name, strlen(cmd->name));
                sys_req(WRITE, COM1, " - ", strlen(" - "));
                sys_req(WRITE, COM1, cmd->description, strlen(cmd->description));
                sys_req(WRITE, COM1, "\r\n", strlen("\r\n"));
                return;
            }
        }
        sys_req(WRITE, COM1, "Command not found.\r\n", strlen("Command not found.\r\n"));
    }
    else
    {
        for (command_t *cmd = commands; cmd->name; cmd++)
        {
            sys_req(WRITE, COM1, cmd->name, strlen(cmd->name));
            sys_req(WRITE, COM1, " - ", strlen(" - "));
            sys_req(WRITE, COM1, cmd->description, strlen(cmd->description));
            sys_req(WRITE, COM1, "\r\n", strlen("\r\n"));
        }
    }
}

// Command for shutting down the OS
void shutdown_command(const char *args)
{
    (void)args; // Mark the parameter as unused
    sys_req(WRITE, COM1, "Are you sure you want to shutdown? (yes/no): ", 46);
    char confirmation[5];
    sys_req(READ, COM1, confirmation, 5); // Read 5 characters
    confirmation[4] = '\0';               // Ensure it's null-terminated

    if (strcmp(confirmation, "yes\n") == 0)
    { // Check for "yes" followed by Enter (\n)
        sys_req(WRITE, COM1, "Shutting down...\r\n", 18);
        should_shutdown = 1; // Set global variable
    }
    else
    {
        sys_req(WRITE, COM1, "Shutdown cancelled.\r\n", 21);
        should_shutdown = 0; // Reset the global variable just in case
    }
}

// Command for setting the date
void set_date_command(const char *date_str)
{
    if (date_str == NULL)
    {
        sys_req(WRITE, COM1, "Enter the date next to the command\r\n", 37);
        return;
    }
    else
    {
        if (strlen(date_str) != 8)
        {
            sys_req(WRITE, COM1, "Invalid date format(MM/DD/YY).\r\n", 33);
            return;
        }

        unsigned char month, day, year;
        month = (date_str[0] - '0') * 10 + (date_str[1] - '0');
        day = (date_str[3] - '0') * 10 + (date_str[4] - '0');
        year = (date_str[6] - '0') * 10 + (date_str[7] - '0');

        // Check for valid month and day
        if (month < 1 || month > 12 || day < 1 || day > 31)
        {
            sys_req(WRITE, COM1, "Invalid date values.\r\n", 23);
            return;
        }

        // Read the current date and time from the RTC
        unsigned char current_month, current_day, current_year;
        current_month = bcd_to_binary(read_rtc(RTC_MONTH));
        current_day = bcd_to_binary(read_rtc(RTC_DAY_OF_MONTH));
        current_year = bcd_to_binary(read_rtc(RTC_YEAR));

        // Check if the current date matches the desired date
        if (current_month == month && current_day == day && current_year == year)
        {
            sys_req(WRITE, COM1, "Date is already set to the desired value.\r\n", 43);
            return;
        }

        // Check if it's a leap year (assuming RTC does not support Feb 29)
        if (month == 2 && day == 29)
        {
            if (!((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
            {
                sys_req(WRITE, COM1, "Invalid date (not a leap year).\r\n", 34);
                return;
            }
            else
            {
                // If it's a leap year, manually adjust the date to Feb 29
                day = 29;
            }
        }

        // Set the new date and time
        write_rtc(RTC_MONTH, binary_to_bcd(month));
        write_rtc(RTC_DAY_OF_MONTH, binary_to_bcd(day));
        write_rtc(RTC_YEAR, binary_to_bcd(year));

        // Verify that the date and time were set correctly
        current_month = bcd_to_binary(read_rtc(RTC_MONTH));
        current_day = bcd_to_binary(read_rtc(RTC_DAY_OF_MONTH));
        current_year = bcd_to_binary(read_rtc(RTC_YEAR));

        if (current_month == month && current_day == day && current_year == year)
        {
            sys_req(WRITE, COM1, "Date set successfully.\r\n", 25);
        }
        else
        {
            sys_req(WRITE, COM1, "Failed to set the date. Can you redo the command\r\n", 23);
        }
    }
}

// Command to set the time in RTC
void set_time_command(const char *time_str)
{
    if (time_str == NULL)
    {
        char msg[] = "Enter the time next to the command\r\n\0";

        sys_req(WRITE, COM1, msg, sizeof(msg));

        return;
    }
    else
    {
        if (strlen(time_str) != 8)
        {

            sys_req(WRITE, COM1, "Invalid time format(hh:mm:ss).\r\n\0", 36);
            return;
        }

        unsigned char hours, minutes, seconds;
        hours = (time_str[0] - '0') * 10 + (time_str[1] - '0');
        minutes = (time_str[3] - '0') * 10 + (time_str[4] - '0');
        seconds = (time_str[6] - '0') * 10 + (time_str[7] - '0');

        if (hours > 23 || minutes > 59 || seconds > 59)
        {

            sys_req(WRITE, COM1, "Invalid time values.\r\n", 23);
            return;
        }

        write_rtc(RTC_HOURS, binary_to_bcd(hours));
        write_rtc(RTC_MINUTES, binary_to_bcd(minutes));
        write_rtc(RTC_SECONDS, binary_to_bcd(seconds));

        sys_req(WRITE, COM1, "Time set successfully.\r\n", 26);
    }
}

// Command for creating a pcb in the format: 'createpcb [name] [class] [priority]'
void create_pcb_command(const char *createpcb_str)
{
    // Missing all 3 attributes entirely
    if (createpcb_str == NULL)
    {
        char err_msg[] = "Invalid format: 'createpcb [name] [class] [priority]'\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    char *tokens[3];                                      // array to store the name, class, and priority
    char *token = strtok((char *)createpcb_str, " \t\n"); // tokenize the first string on space, tab, or newline

    int num_tokens = 0;

    // Tokenize what is left of createpcb_str
    while (token != NULL && num_tokens < 3)
    {
        tokens[num_tokens++] = token;
        token = strtok(NULL, " \t\n");
    }

    // Either too many attributes or not enough attributes
    if (num_tokens != 3)
    {
        char err_msg[] = "Please provide the name, class, and priority\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // Process name exceeds length limit 8
    if (strlen(tokens[0]) > 8)
    {
        char err_msg[] = "Process names must be no more than 8 characters long\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // Process with the same name already exists
    if (pcb_find(tokens[0]) != NULL)
    {
        char err_msg[] = "A process by that name already exists\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    int class;

    // Determine the class
    if (strcmp(tokens[1], "user") == 0)
    {
        class = USER_APP;
    }
    else if (strcmp(tokens[1], "system") == 0)
    {
        class = SYSTEM_PROCESS;
    }
    // Invalid class entry
    else
    {
        char err_msg[] = "Process class must be either 'user' or 'system'\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    int priority;

    // atoi(str) returns '0' if there is no int in str - check for '0' before conversion
    if (strcmp(tokens[2], "0") != 0)
    {
        priority = atoi(tokens[2]);

        // Priority entry falls outside of the accepted range (a zero here indicates an invalid entry)
        if (priority < 1 || priority > 9)
        {
            char err_msg[] = "Process priority must be an integer between 0 and 9\r\n\0";
            sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

            return;
        }
    }
    else
    {
        priority = 0;
    }

    pcb_setup(tokens[0], class, priority);

    char success_msg[] = "PCB successfully created\r\n\0";
    sys_req(WRITE, COM1, success_msg, sizeof(success_msg));
}

// Command for showing a pcb in the format: 'showpcb [name]'
void show_pcb_command(const char *showpcb_str)
{
    // PCB name not included
    if (showpcb_str == NULL)
    {
        char err_msg[] = "Include the name of the process: 'showpcb [name]'";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg) - 1);

        return;
    }
    // Given PCB name exceeds length limit
    if (strlen(showpcb_str) > 8)
    {
        char err_msg[] = "Invalid process name (no more than 8 characters)";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg) - 1);

        return;
    }

    struct pcb *target_pcb = pcb_find(showpcb_str); // search for pcb by name

    // PCB not found
    if (target_pcb == NULL)
    {
        char err_msg[] = "Process not found";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg) - 1);

        return;
    }
    // PCB was found
    else
    {
        // Use an array for class and state for easier lookup
        char *classes[] = {"User Application", "System Process"};
        char *states[] = {"Ready", "Blocked"};
        char *statuses[] = {"Not Suspended", "Suspended"};

        // Display PCB name
        sys_req(WRITE, COM1, "Name: ", 6);
        sys_req(WRITE, COM1, target_pcb->process_name, sizeof(target_pcb->process_name) - 1);
        sys_req(WRITE, COM1, "\r\n", 2);

        // Display PCB class
        sys_req(WRITE, COM1, "Class: ", 7);
        sys_req(WRITE, COM1, classes[target_pcb->process_class], strlen(classes[target_pcb->process_class]));
        sys_req(WRITE, COM1, "\r\n", 2);

        // Display PCB execution state
        sys_req(WRITE, COM1, "State: ", 7);
        sys_req(WRITE, COM1, states[target_pcb->execution_state], strlen(states[target_pcb->execution_state]));
        sys_req(WRITE, COM1, "\r\n", 2);

        // Display PCB suspension status
        sys_req(WRITE, COM1, "Status: ", 8);
        sys_req(WRITE, COM1, statuses[target_pcb->dispatching_state], strlen(statuses[target_pcb->dispatching_state]));
        sys_req(WRITE, COM1, "\r\n", 2);

        // Display PCB priority using sys_req
        char priority_msg[] = "Priority: x\r\n";
        priority_msg[10] = '0' + target_pcb->process_priority; // Convert integer to character
        sys_req(WRITE, COM1, priority_msg, sizeof(priority_msg) - 1);
    }
}

// Function to show the PCBs in a given queue
void show_queue(struct queue *q)
{
    if (q != NULL && q->front != NULL)
    {
        struct pcb *current = q->front;
        while (current != NULL)
        {
            // Use the show_pcb_command function to print each PCB's details
            show_pcb_command(current->process_name);
            current = current->next;
            char msg[] = "~~~~~~~~~~~~\r\n\0";
            sys_req(WRITE, COM1, msg, sizeof(msg));
        }
    }
    else
    {
        char msg[] = "Queue is empty.\r\n\0";
        sys_req(WRITE, COM1, msg, sizeof(msg));
    }
}

// Command for showing all the pcbs in All the queues
void show_all_pcbs_command()
{
    show_ready_pcbs_command();
    show_blocked_pcbs_command();
}

// show ready pcbs
void show_ready_pcbs_command()
{
    char readyMsg[] = "---------Ready Queue:---------\r\n\0";
    sys_req(WRITE, COM1, readyMsg, sizeof(readyMsg));
    show_queue(get_ready_q());

    char suspReadyMsg[] = "---------Suspended Ready Queue:---------\r\n\0";
    sys_req(WRITE, COM1, suspReadyMsg, sizeof(suspReadyMsg));
    show_queue(get_susp_ready_q());
}

// show blocked pcbs
void show_blocked_pcbs_command()
{
    char blockedMsg[] = "---------Blocked Queue:---------\r\n\0";
    sys_req(WRITE, COM1, blockedMsg, sizeof(blockedMsg));
    show_queue(get_blocked_q());

    char suspBlockedMsg[] = "---------Suspended Blocked Queue:---------\r\n\0";
    sys_req(WRITE, COM1, suspBlockedMsg, sizeof(suspBlockedMsg));
    show_queue(get_susp_blocked_q());
}

void delete_pcb_command(const char *name)
{
    // Check if a name was provided
    if (name == NULL || strlen(name) == 0)
    {
        char msg[] = "Process not found\r\n\0";
        sys_req(WRITE, COM1, msg, sizeof(msg));
        return;
    }

    struct pcb *pcb = pcb_find(name);

    if (pcb != NULL)
    {
        if (pcb->process_class == SYSTEM_PROCESS)
        {
            char err_msg[] = "Cannot delete a System Process\r\n\0";
            sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
            return;
        }
        // Remove the PCB from its queue
        if (pcb_remove(pcb) == 0)
        {
            // Free the associated memory
            pcb_free(pcb);
            char succ_msg[] = "PCB deleted successfully\r\n\0";
            sys_req(WRITE, COM1, succ_msg, sizeof(succ_msg));
        }
        else
        {
            char err_msg[] = "Error deleting PCB\r\n\0";
            sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
        }
    }
    else
    {
        char err_msg[] = "Process not found\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
    }
}

void suspend_pcb_command(const char *name)
{
    // Check if a name was provided
    if (name == NULL || strlen(name) == 0)
    {
        char err_msg[] = "Usage: suspendpcb [name]\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
    }

    struct pcb *pcb = pcb_find(name);

    if (pcb != NULL)
    {
        // Check if the PCB is a system process
        if (pcb->process_class == SYSTEM_PROCESS)
        {
            char err_msg[] = "System processes cannot be suspended.\r\n\0";
            sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
        }
        else
        {
            // Check if the PCB is already suspended
            if (pcb->dispatching_state == SUSPENDED)
            {
                char err_msg[] = "PCB is already in a suspended state.\r\n\0";
                sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
            }
            else
            {
                // Move the PCB to the appropriate suspended queue
                pcb_remove(pcb); // Remove from the current queue
                
                // Set the PCB's dispatching state to SUSPENDED
                pcb->dispatching_state = SUSPENDED;

                pcb_insert(pcb); // Insert into the suspended queue

                char succ_msg[] = "PCB suspended successfully.\r\n\0";
                sys_req(WRITE, COM1, succ_msg, sizeof(succ_msg));
            }
        }
    }
    else
    {
        sys_req(WRITE, COM1, "PCB not found.\r\n", 17);
    }
}

void resume_pcb_command(const char *resumepcb_str)
{
    // PCB name not included
    if (resumepcb_str == NULL)
    {
        char err_msg[] = "Include the name of the process: 'resumepcb [name]'\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }
    // Given PCB name exceeds length limit
    if (strlen(resumepcb_str) > 8)
    {
        char err_msg[] = "Invalid process name (no more than 8 characters)\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    struct pcb *pcb_to_resume = pcb_find(resumepcb_str); // Find the PCB by process name

    // PCB doesn't exist
    if (pcb_to_resume == NULL)
    {
        char err_msg[] = "Process not found\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // Remove the PCB from its current queue
    if (pcb_remove(pcb_to_resume) == -1)
    {
        char err_msg[] = "Process failed to resume\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // Update the dispatching state to NOT_SUSPENDED
    pcb_to_resume->dispatching_state = NOT_SUSPENDED;

    // Insert the PCB into the appropriate queue
    pcb_insert(pcb_to_resume);

    char success_msg[] = "PCB successfully resumed\r\n\0";
    sys_req(WRITE, COM1, success_msg, sizeof(success_msg));
}

void block_pcb_command(const char *blockpcb_str)
{
    // PCB name not included
    if (blockpcb_str == NULL)
    {
        char err_msg[] = "Include the name of the process: 'blockpcb [name]'\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }
    // Given PCB name exceeds length limit
    if (strlen(blockpcb_str) > 8)
    {
        char err_msg[] = "Invalid process name (no more than 8 characters)\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    struct pcb *pcb_to_block = pcb_find(blockpcb_str); // Find the PCB by process name

    // PCB doesn't exist
    if (pcb_to_block == NULL)
    {
        char err_msg[] = "Process not found\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // PCB is a system process
    if (pcb_to_block->process_class == SYSTEM_PROCESS)
    {
        char err_msg[] = "Cannot block a System Process\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // Remove the PCB from its current queue
    if (pcb_remove(pcb_to_block) == -1)
    {
        char err_msg[] = "Process failed to resume\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // Update the execution state to BLOCKED
    pcb_to_block->execution_state = BLOCKED;

    // Insert the PCB into the appropriate queue
    pcb_insert(pcb_to_block);

    char success_msg[] = "PCB successfully blocked\r\n\0";
    sys_req(WRITE, COM1, success_msg, sizeof(success_msg));
}

void unblock_pcb_command(const char *unblockpcb_str)
{
    // PCB name not included
    if (unblockpcb_str == NULL)
    {
        char err_msg[] = "Include the name of the process: 'unblockpcb [name]'\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }
    // Given PCB name exceeds length limit
    if (strlen(unblockpcb_str) > 8)
    {
        char err_msg[] = "Invalid process name (no more than 8 characters)\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    struct pcb *pcb_to_unblock = pcb_find(unblockpcb_str); // Find the PCB by process name

    // PCB doesn't exist
    if (pcb_to_unblock == NULL)
    {
        char err_msg[] = "Process not found\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // Remove the PCB from its current queue
    if (pcb_remove(pcb_to_unblock) == -1)
    {
        char err_msg[] = "Process failed to resume\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));

        return;
    }

    // Update the execution state to BLOCKED
    pcb_to_unblock->execution_state = READY;

    // Insert the PCB into the appropriate queue
    pcb_insert(pcb_to_unblock);

    char success_msg[] = "PCB successfully unblocked\r\n\0";
    sys_req(WRITE, COM1, success_msg, sizeof(success_msg));
}

// Command for setting the priority of a PCB in the format: 'setpcbpriority [name] [newpriority]'
void set_pcb_priority_command(const char *args)
{
    if (args == NULL)
    {
        char err_msg[] = "Invalid format: 'setpcbpriority [name] [newpriority]'\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
        return;
    }

    char *tokens[2];                             // Array to store the name and newpriority
    char *token = strtok((char *)args, " \t\n"); // Tokenize the first string on space, tab, or newline

    int num_tokens = 0;

    // Tokenize what is left of args
    while (token != NULL && num_tokens < 2)
    {
        tokens[num_tokens++] = token;
        token = strtok(NULL, " \t\n");
    }

    // Either too many attributes or not enough attributes
    if (num_tokens != 2)
    {
        char err_msg[] = "Please provide the name and newpriority\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
        return;
    }

    // Process name exceeds length limit 8
    if (strlen(tokens[0]) > 8)
    {
        char err_msg[] = "Process names must be no more than 8 characters long\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
        return;
    }

    int new_priority = atoi(tokens[1]);

    int result = pcb_set_priority(tokens[0], new_priority);

    if (result == -1)
    {
        char err_msg[] = "Process not found\r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
    }
    else if (result == -2)
    {
        char err_msg[] = "Invalid priority, new priority must be an integer between 0 and 9 \r\n\0";
        sys_req(WRITE, COM1, err_msg, sizeof(err_msg));
    }
    else if (result == 0)
    {
        char success_msg[] = "PCB priority successfully updated\r\n\0";
        sys_req(WRITE, COM1, success_msg, sizeof(success_msg));
    }
}

void yield_command(const char *args){
    (void)args;
    sys_req(WRITE, COM1, "Yielding...",12);
    sys_req(IDLE);
}

void loadR3_command(const char *args){
    (void)args;
    sys_req(WRITE, COM1, "Loading R3...",14);
}
void comhand(void)
{
    for (;;)
    {
        sys_req(WRITE, COM1, "(path)===> $ ", 14);

        char buf[100] = {0};
        int nread = sys_req(READ, COM1, buf, sizeof(buf) - 1); // -1 to ensure space for null-terminator
        sys_req(WRITE, COM1, "Your command : ", 16);
        sys_req(WRITE, COM1, buf, nread);
        trim_input(buf);

        // Find the first space in buf
        char *space = NULL;
        for (int i = 0; i < nread; i++)
        {
            if (buf[i] == ' ')
            {
                space = &buf[i];
                break;
            }
        }

        const char *command;
        const char *args = NULL;

        if (space)
        {
            *space = '\0'; // Split the string into command and args
            command = buf;
            args = space + 1;
        }
        else
        {
            command = buf;
        }

        // Process the command
        int found = 0;
        for (command_t *cmd = commands; cmd->name; cmd++)
        {
            if (strcmp(command, cmd->name) == 0)
            {
                cmd->handler(args);
                found = 1;
                break;
            }
        }

        if (!found)
        {
            sys_req(WRITE, COM1, "Unvalid command. Use help command.\r\n", 40);
        }

        // Check for shutdown command and confirmation
        if (should_shutdown)
        {
            return; // Exit the loop
        }
    }
}

