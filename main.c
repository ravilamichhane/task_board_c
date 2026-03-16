#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 50
#define MAX_PROJECTS 20
#define MAX_TASKS 200
#define MAX_MEMBERS 200

#define ADMIN_USERNAME "admin"

typedef enum
{
    TODO,
    IN_PROGRESS,
    DONE
} TaskStatus;

typedef enum
{
    LOW,
    MEDIUM,
    HIGH
} Priority;

typedef struct
{
    int id;
    char name[50];
    char email[50];

} User;

typedef struct
{
    int id;
    char name[50];
    char description[200];
} Project;

typedef struct
{
    int id;
    char title[50];
    char description[200];
    TaskStatus status;
    Priority priority;
    int assigned_user_id, project_id;
    char due_date[11];
} Task;

typedef struct
{
    User users[MAX_USERS];
    int user_count;
    Project projects[MAX_PROJECTS];
    int project_count;
    Task tasks[MAX_TASKS];
    int task_count;
    int logged_in_user_id;
    int is_admin;

} AppData;

void admin_dashboard(AppData *data);
void user_menu(AppData *data);
void project_menu(AppData *data);
Task *find_task_by_id(AppData *data, int id);
void list_tasks(AppData *data, int project_id);
void view_task_details(AppData *data, int task_id);
const char *status_to_string(TaskStatus s);
const char *priority_to_string(Priority p);

// Helper functions for input
void flush_input(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

void read_string(char *buffer, int max_len)
{
    if (fgets(buffer, max_len, stdin) != NULL)
        buffer[strcspn(buffer, "\n")] = '\0';
    else
        buffer[0] = '\0';
}

void read_string_with_prompt(const char *prompt, char *buffer, int max_len)
{
    printf("%s", prompt);
    read_string(buffer, max_len);
}

void press_enter_to_continue(void)
{
    printf("\n  Press Enter to continue...");
    flush_input();
}

int read_int()
{
    char buffer[32];
    int x;
    for (;;)
    {
        if (scanf("%d", &x) == 1)
        {
            flush_input();
            return x;
        }
        else
        {
            printf("  Invalid input. Please enter a number.\n");
            flush_input();
        }
    }
}

int read_int_with_prompt(const char *prompt)
{
    printf("%s", prompt);
    return read_int();
}

void edit_field(const char *label, char *field, int max_len)
{
    char temp[200];
    int len = max_len < (int)sizeof(temp) ? max_len : (int)sizeof(temp);
    printf("  Current %s: %s\n  New %s (Enter to keep): ", label, field, label);
    read_string(temp, len);
    if (strlen(temp) > 0)
        strcpy(field, temp);
}

// Array read/write helpers

int save_records(const char *filename, const void *records, int count, size_t sz)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        perror("  Error saving");
        return 0;
    }
    fwrite(&count, sizeof(int), 1, fp);
    fwrite(records, sz, (size_t)count, fp);
    fclose(fp);
    return 1;
}

int load_records(const char *filename, void *records, int *count, int max, size_t sz)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        *count = 0;
        return 1;
    }
    if (fread(count, sizeof(int), 1, fp) != 1)
    {
        *count = 0;
        fclose(fp);
        return 0;
    }
    if (*count > max)
        *count = max;
    fread(records, sz, (size_t)*count, fp);
    fclose(fp);
    return 1;
}

void save_all(AppData *d)
{
    save_records("users.dat", d->users, d->user_count, sizeof(User));
    save_records("projects.dat", d->projects, d->project_count, sizeof(Project));
    save_records("tasks.dat", d->tasks, d->task_count, sizeof(Task));
}
void load_all(AppData *d)
{
    load_records("users.dat", d->users, &d->user_count, MAX_USERS, sizeof(User));
    load_records("projects.dat", d->projects, &d->project_count, MAX_PROJECTS, sizeof(Project));
    load_records("tasks.dat", d->tasks, &d->task_count, MAX_TASKS, sizeof(Task));
}

int add_user(AppData *data)
{
    if (data->user_count >= MAX_USERS)
    {
        printf("  Error: Max user limit reached.\n");
        return 0;
    }
    User *u = &data->users[data->user_count];
    u->id = (data->user_count > 0) ? data->users[data->user_count - 1].id + 1 : 1;

    read_string_with_prompt("  Enter name : ", u->name, sizeof(u->name));
    read_string_with_prompt("  Enter email: ", u->email, sizeof(u->email));
    data->user_count++;
    printf("  User #%d '%s' added.\n", u->id, u->name);
    save_all(data);
    return 1;
}

int list_users(const AppData *data)
{
    if (data->user_count == 0)
    {
        printf("  No users found.\n");
        return 0;
    }
    printf("\n  Users:\n");
    printf("\n  %-5s %-20s %-25s\n", "ID", "Name", "Email");
    printf("  %-5s %-20s %-25s\n", "-----", "--------------------", "-------------------------");
    for (int i = 0; i < data->user_count; i++)
        printf("  %-5d %-20s %-25s\n", data->users[i].id, data->users[i].name, data->users[i].email);
    return 1;
}

void remove_array_element(void *array, int *count, int index, size_t elem_size)
{
    char *base = (char *)array;
    for (int j = index; j < *count - 1; j++)
        memcpy(base + j * elem_size, base + (j + 1) * elem_size, elem_size);
    (*count)--;
}

User *find_user_by_id(const AppData *data, int id)
{
    for (int i = 0; i < data->user_count; i++)
        if (data->users[i].id == id)
            return &data->users[i];
    return NULL;
}

const char *get_user_name(const AppData *data, int id)
{
    if (id == 0)
        return "unassigned";
    for (int i = 0; i < data->user_count; i++)
        if (data->users[i].id == id)
            return data->users[i].name;
    return "unknown";
}

int edit_user(AppData *data, int id)
{
    User *u = find_user_by_id(data, id);
    if (!u)
    {
        printf("  User #%d not found.\n", id);
        return 0;
    }
    edit_field("name", u->name, sizeof(u->name));
    edit_field("email", u->email, sizeof(u->email));
    printf("  User #%d updated.\n", id);
    save_all(data);
    return 1;
}

int delete_user(AppData *data, int id)
{
    for (int i = 0; i < data->task_count; i++)
        if (data->tasks[i].assigned_user_id == id)
        {
            data->tasks[i].assigned_user_id = 0; /* unassign tasks instead of blocking deletion */
        }
    for (int i = 0; i < data->user_count; i++)
        if (data->users[i].id == id)
        {
            remove_array_element(data->users, &data->user_count, i, sizeof(User));
            printf("  User #%d deleted.\n", id);
            save_all(data);
            return 1;
        }
    printf("  User #%d not found.\n", id);
    return 0;
}

int add_project(AppData *data)
{
    if (data->project_count >= MAX_PROJECTS)
    {
        printf("  Error: Max project limit reached.\n");
        return 0;
    }
    Project *p = &data->projects[data->project_count];
    p->id = (data->project_count > 0) ? data->projects[data->project_count - 1].id + 1 : 1;

    read_string_with_prompt("  Enter name : ", p->name, sizeof(p->name));
    read_string_with_prompt("  Enter description: ", p->description, sizeof(p->description));
    data->project_count++;
    printf("  Project #%d '%s' added.\n", p->id, p->name);
    save_all(data);
    return 1;
}

void list_projects(const AppData *data)
{
    if (data->project_count == 0)
    {
        printf("  No projects found.\n");
        return;
    }
    printf("\n  Projects:\n");
    printf("\n  %-5s %-20s %-30s\n", "ID", "Name", "Description");
    printf("  %-5s %-20s %-30s\n", "-----", "--------------------", "------------------------------");
    for (int i = 0; i < data->project_count; i++)
        printf("  %-5d %-20s %-30s\n", data->projects[i].id, data->projects[i].name, data->projects[i].description);
}

Project *find_project_by_id(const AppData *data, int id)
{
    for (int i = 0; i < data->project_count; i++)
        if (data->projects[i].id == id)
            return &data->projects[i];
    return NULL;
}

void edit_project(AppData *data, int id)
{
    Project *p = find_project_by_id(data, id);
    if (!p)
    {
        printf("  Project #%d not found.\n", id);
        return;
    }
    edit_field("name", p->name, sizeof(p->name));
    edit_field("description", p->description, sizeof(p->description));
    printf("  Project #%d updated.\n", id);
    save_all(data);
}

int delete_project(AppData *data, int id)
{
    for (int i = data->task_count - 1; i >= 0; i--) /* backward iteration for safe removal */
        if (data->tasks[i].project_id == id)
        {
            remove_array_element(data->tasks, &data->task_count, i, sizeof(Task));
        }
    for (int i = 0; i < data->project_count; i++)
        if (data->projects[i].id == id)
        {
            remove_array_element(data->projects, &data->project_count, i, sizeof(Project));
            printf("  Project #%d and its tasks deleted.\n", id);
            save_all(data);
            return 1;
        }
    printf("  Project #%d not found.\n", id);
    return 0;
}

const char *status_to_string(TaskStatus s)
{
    switch (s)
    {
    case TODO:
        return "TODO";
    case IN_PROGRESS:
        return "IN PROGRESS";
    case DONE:
        return "DONE";
    default:
        return "UNKNOWN";
    }
}

const char *priority_to_string(Priority p)
{
    switch (p)
    {
    case LOW:
        return "LOW";
    case MEDIUM:
        return "MED";
    case HIGH:
        return "HIGH";
    default:
        return "???";
    }
}

Task *find_task_by_id(AppData *data, int id)
{
    for (int i = 0; i < data->task_count; i++)
        if (data->tasks[i].id == id)
            return &data->tasks[i];
    return NULL;
}

void list_tasks(AppData *data, int project_id)
{
    int found = 0;
    printf("\n  %-4s %-18s %-12s %-5s %-12s %-11s\n",
           "ID", "Title", "Status", "Pri", "Assigned To", "Due Date");
    printf("  %-4s %-18s %-12s %-5s %-12s %-11s\n",
           "----", "------------------", "------------", "-----", "------------", "-----------");

    for (int i = 0; i < data->task_count; i++)
    {
        if (data->tasks[i].project_id != project_id)
            continue;

        printf("  %-4d %-18.18s %-12s %-5s %-12.12s %-11s\n",
               data->tasks[i].id, data->tasks[i].title,
               status_to_string(data->tasks[i].status),
               priority_to_string(data->tasks[i].priority),
               get_user_name(data, data->tasks[i].assigned_user_id),
               data->tasks[i].due_date);
        found++;
    }

    if (!found)
        printf("  No tasks in this project.\n");
}

void view_task_details(AppData *data, int task_id)
{
    Task *t = find_task_by_id(data, task_id);
    if (!t)
    {
        printf("  Task #%d not found.\n", task_id);
        return;
    }

    printf("\n  --- Task #%d Details ---\n", t->id);
    printf("  Title      : %s\n", t->title);
    printf("  Description: %s\n", t->description);
    printf("  Status     : %s\n", status_to_string(t->status));
    printf("  Priority   : %s\n", priority_to_string(t->priority));
    printf("  Assigned To: %s\n", get_user_name(data, t->assigned_user_id));
    printf("  Due Date   : %s\n", t->due_date);
}

void print_pending_tasks(AppData *data)
{
    int found = 0;
    printf("\n  Pending Tasks Assigned to You:\n");
    printf("\n  %-4s %-18s %-12s %-5s %-12s %-11s\n",
           "ID", "Title", "Status", "Pri", "Assigned To", "Due Date");
    printf("  %-4s %-18s %-12s %-5s %-12s %-11s\n",
           "----", "------------------", "------------", "-----", "------------", "-----------");
    for (int i = 0; i < data->task_count; i++)
    {
        if (data->tasks[i].assigned_user_id == data->logged_in_user_id &&
            data->tasks[i].status != DONE)
        {
            printf("  %-4d %-18.18s %-12s %-5s %-12.12s %-11s\n",
                   data->tasks[i].id, data->tasks[i].title,
                   status_to_string(data->tasks[i].status), priority_to_string(data->tasks[i].priority),
                   get_user_name(data, data->tasks[i].assigned_user_id), data->tasks[i].due_date);
            found++;
        }
    }
    if (!found)
        printf("  No pending tasks assigned to you.\n");
}

void add_task(AppData *data, int project_id)
{
    if (data->task_count >= MAX_TASKS)
    {
        printf("  Error: Max task limit reached.\n");
        return;
    }
    Task *t = &data->tasks[data->task_count];
    t->id = (data->task_count > 0) ? data->tasks[data->task_count - 1].id + 1 : 1;
    t->project_id = project_id;

    read_string_with_prompt("  Enter title : ", t->title, sizeof(t->title));
    read_string_with_prompt("  Enter description: ", t->description, sizeof(t->description));
    t->status = TODO;
    int p = read_int_with_prompt("  Priority (0=Low, 1=Med, 2=High): ");
    switch (p)
    {
    case LOW:
        t->priority = LOW;
        break;
    case MEDIUM:
        t->priority = MEDIUM;
        break;
    case HIGH:
        t->priority = HIGH;
        break;
    default:
        t->priority = MEDIUM;
        printf("  Invalid. Defaulting to MEDIUM.\n");
        break;
    }

    list_users(data);
    t->assigned_user_id = read_int_with_prompt("  Assign to user ID (0=unassigned): ");

    read_string_with_prompt("  Due date (YYYY-MM-DD) : ", t->due_date, sizeof(t->due_date));
    data->task_count++;
    printf("  Task #%d '%s' created in TODO.\n", t->id, t->title);

    save_all(data);
}

void edit_task(AppData *data, int task_id)
{
    Task *t = find_task_by_id(data, task_id);
    if (!t)
    {
        printf("  Task #%d not found.\n", task_id);
        return;
    }
    edit_field("title", t->title, sizeof(t->title));
    edit_field("description", t->description, sizeof(t->description));
    int p = read_int_with_prompt("  Priority (0=Low, 1=Med, 2=High): ");
    switch (p)
    {
    case LOW:
        t->priority = LOW;
        break;
    case MEDIUM:
        t->priority = MEDIUM;
        break;
    case HIGH:
        t->priority = HIGH;
        break;
    default:
        printf("  Invalid. Keeping current priority.\n");
        break;
    }

    list_users(data);
    int new_user_id = read_int_with_prompt("  Assign to user ID (0=unassigned): ");
    if (new_user_id != 0 && !find_user_by_id(data, new_user_id))
        printf("  User ID not found. Keeping current assignment.\n");
    else
        t->assigned_user_id = new_user_id;

    edit_field("due date (YYYY-MM-DD)", t->due_date, sizeof(t->due_date));
    printf("  Task #%d updated.\n", task_id);
    save_all(data);
}

void delete_task(AppData *data, int task_id)
{
    for (int i = 0; i < data->task_count; i++)
        if (data->tasks[i].id == task_id)
        {
            remove_array_element(data->tasks, &data->task_count, i, sizeof(Task));
            printf("  Task #%d deleted.\n", task_id);
            save_all(data);
            return;
        }
    printf("  Task #%d not found.\n", task_id);
}

void change_task_status(AppData *data, int task_id)
{
    Task *t = find_task_by_id(data, task_id);
    if (!t)
    {
        printf("  Task #%d not found.\n", task_id);
        return;
    }
    printf("  Current status: %s\n", status_to_string(t->status));
    printf("  New status (0=TODO, 1=In Progress, 2=Done): ");
    int s = read_int();
    switch (s)
    {
    case TODO:
        t->status = TODO;
        break;
    case IN_PROGRESS:
        t->status = IN_PROGRESS;
        break;
    case DONE:
        t->status = DONE;
        break;
    default:
        printf("  Invalid status. Keeping current status.\n");
        return;
    }
    save_all(data);
}

void assign_task(AppData *data, int task_id, int user_id)
{
    Task *t = find_task_by_id(data, task_id);
    if (!t)
    {
        printf("  Task #%d not found.\n", task_id);
        return;
    }
    if (user_id != 0 && !find_user_by_id(data, user_id))
    {
        printf("  User ID not found. Task not assigned.\n");
        return;
    }
    t->assigned_user_id = user_id;
    printf("  Task #%d assigned to user ID %d.\n", task_id, user_id);
    save_all(data);
}

void save_all(AppData *d);
void load_all(AppData *d);

void admin_menu(AppData *data);
void user_dashboard(AppData *data);
void kanban(AppData *data, int project_id);

int login(AppData *data);

void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int main(void)
{
    AppData data = {.user_count = 0, .project_count = 0, .task_count = 0, .logged_in_user_id = 0, .is_admin = 0};
    load_all(&data);
    printf("  Data loaded: %d user(s), %d project(s), %d task(s)\n",
           data.user_count, data.project_count, data.task_count);

    int running = 1;
    while (running)
    {
        clear_screen();
        data.logged_in_user_id = 0;
        data.is_admin = 0;
        int result = login(&data);
        if (result == -1)
            running = 0;
        else if (result == 1)
        {
            if (data.is_admin)
                admin_dashboard(&data);
            else
                user_dashboard(&data);
        }
        else
            press_enter_to_continue();
    }
    return 0;
}

int login(AppData *data)
{
    char username[50];

    read_string_with_prompt("  Enter username (or 'exit' to quit): ", username, sizeof(username));
    if (strcmp(username, "exit") == 0)
        return -1;
    if (strcmp(username, ADMIN_USERNAME) == 0)
    {
        data->is_admin = 1;
        data->logged_in_user_id = -1;
        printf("  Welcome, Administrator!\n");
        return 1;
    }
    for (int i = 0; i < data->user_count; i++)
        if (strcmp(data->users[i].name, username) == 0)
        {
            data->is_admin = 0;
            data->logged_in_user_id = data->users[i].id;
            printf("  Welcome, %s!\n", data->users[i].name);
            return 1;
        }
    printf("  User '%s' not found. Please contact admin.\n", username);
    return 0;
}

void display_task_board(AppData *data, int project_id)
{
    const char *status_names[] = {"To Do", "In Progress", "Done"};
    printf("\n  Task Board for Project #%d: %s\n", project_id,
           find_project_by_id(data, project_id)->name);
    for (int s = 0; s < 3; s++)
    {
        printf("\n  --- %s ---\n", status_names[s]);
        int found = 0;
        for (int i = 0; i < data->task_count; i++)
        {
            if (data->tasks[i].project_id == project_id && data->tasks[i].status == s)
            {
                printf("  [%d] %s (Pri: %s, Assigned: %s, Due: %s)\n",
                       data->tasks[i].id, data->tasks[i].title, priority_to_string(data->tasks[i].priority),
                       get_user_name(data, data->tasks[i].assigned_user_id), data->tasks[i].due_date);
                found++;
            }
        }
        if (!found)
            printf("  No tasks in this category.\n");
    }
}

void admin_dashboard(AppData *data)
{
    int ch;
    do
    {
        clear_screen();
        printf("\n  +----------------------------------+\n");
        printf("  |      ADMIN DASHBOARD             |\n");
        printf("  +----------------------------------+\n");
        printf("  |  1. User Management              |\n");
        printf("  |  2. Project Management           |\n");
        printf("  |  0. Logout                       |\n");
        printf("  +----------------------------------+\n");
        ch = read_int_with_prompt("  Enter choice: ");
        switch (ch)
        {
        case 1:
            user_menu(data);
            break;
        case 2:
            project_menu(data);
            break;
        case 0:
            break;
        default:
            printf("  Invalid choice.\n");
            press_enter_to_continue();
        }
    } while (ch != 0);
}

void user_menu(AppData *data)
{
    int ch;
    do
    {
        clear_screen();
        printf("\n  ===== USER MANAGEMENT =====\n");
        printf("  1. Add User\n  2. List Users\n  3. Edit User\n  4. Delete User\n  0. Back\n");
        ch = read_int_with_prompt("\n  Enter choice: ");
        switch (ch)
        {
        case 1:
            add_user(data);
            press_enter_to_continue();
            break;
        case 2:
            list_users(data);
            press_enter_to_continue();
            break;
        case 3:
        {
            list_users(data);
            int id = read_int_with_prompt("  Enter User ID to edit: ");
            edit_user(data, id);
            press_enter_to_continue();
            break;
        }
        case 4:
        {
            list_users(data);
            int id = read_int_with_prompt("  Enter User ID to delete: ");
            delete_user(data, id);
            press_enter_to_continue();
            break;
        }
        case 0:
            break;
        default:
            printf("  Invalid choice.\n");
            press_enter_to_continue();
        }
    } while (ch != 0);
}

void project_menu(AppData *data)
{
    int ch;
    do
    {
        clear_screen();
        printf("\n  ===== PROJECT MANAGEMENT =====\n");
        printf("  1. Add Project\n  2. List Projects\n  3. Edit Project\n  4. Delete Project\n  0. Back\n");
        ch = read_int_with_prompt("\n  Enter choice: ");
        switch (ch)
        {
        case 1:
            add_project(data);
            press_enter_to_continue();
            break;
        case 2:
            list_projects(data);
            press_enter_to_continue();
            break;
        case 3:
        {
            list_projects(data);
            int id = read_int_with_prompt("  Enter Project ID to edit: ");
            edit_project(data, id);
            press_enter_to_continue();
            break;
        }
        case 4:
        {
            list_projects(data);
            int id = read_int_with_prompt("  Enter Project ID to delete: ");
            delete_project(data, id);
            press_enter_to_continue();
            break;
        }
        case 0:
            break;
        default:
            printf("  Invalid choice.\n");
            press_enter_to_continue();
        }
    } while (ch != 0);
}

void interactive_board(AppData *data, int project_id)
{
    Project *proj = find_project_by_id(data, project_id);
    if (!proj)
    {
        printf("  Project not found.\n");
        press_enter_to_continue();
        return;
    }
    int ch;
    do
    {
        clear_screen();
        display_task_board(data, project_id);

        printf("\n  --- Board Actions ---\n");
        printf("  1. Add Task\n  2. Update Task Status\n  3. Assign Task\n");
        printf("  4. Edit Task\n  5. Delete Task\n  6. View Task Details\n  0. Back\n");
        ch = read_int_with_prompt("\n  Enter choice: ");

        switch (ch)
        {
        case 1:
            add_task(data, project_id);
            press_enter_to_continue();
            break;
        case 2:
        {
            list_tasks(data, project_id);
            int task_id = read_int_with_prompt("  Enter Task ID to update status: ");
            change_task_status(data, task_id);
            press_enter_to_continue();
            break;
        }
        case 3:
        {
            list_tasks(data, project_id);
            int task_id = read_int_with_prompt("  Enter Task ID to assign: ");
            list_users(data);
            int user_id = read_int_with_prompt("  Enter User ID to assign to (0 to unassign): ");
            assign_task(data, task_id, user_id);
            press_enter_to_continue();
            break;
        }
        case 4:
        {
            list_tasks(data, project_id);
            int task_id = read_int_with_prompt("  Enter Task ID to edit: ");
            edit_task(data, task_id);
            press_enter_to_continue();
            break;
        }
        case 5:
        {
            list_tasks(data, project_id);
            int task_id = read_int_with_prompt("  Enter Task ID to delete: ");
            delete_task(data, task_id);
            press_enter_to_continue();
            break;
        }
        case 6:
        {
            list_tasks(data, project_id);
            int task_id = read_int_with_prompt("  Enter Task ID to view details: ");
            view_task_details(data, task_id);
            press_enter_to_continue();
            break;
        }
        case 0:
            break;

        default:
            printf("  Invalid choice.\n");
            press_enter_to_continue();
        }
    } while (ch != 0);
}

void user_dashboard(AppData *data)
{
    int uid = data->logged_in_user_id;
    const char *name = get_user_name(data, uid);
    int ch;

    do
    {
        clear_screen();
        printf("\n  +----------------------------------+\n");
        printf("  |      USER DASHBOARD              |\n");
        printf("  +----------------------------------+\n");
        printf("  |  Welcome, %s!                   |\n", name);
        printf("  +----------------------------------+\n");

        print_pending_tasks(data);
        printf("\n  --- Projects ---\n");
        list_projects(data);

        printf("\n  1. Open Project (Board)\n  0. Logout\n");
        ch = read_int_with_prompt("\n  Enter choice: ");
        switch (ch)
        {
        case 1:
        {

            int pid = read_int_with_prompt("  Enter Project ID to open: ");
            if (find_project_by_id(data, pid))
                interactive_board(data, pid);
            else
            {
                printf("  Project #%d not found.\n", pid);
                press_enter_to_continue();
            }
            break;
        }
        case 0:
            break;
        default:
            printf("  Invalid choice.\n");
            press_enter_to_continue();
        }
    } while (ch != 0);
}
