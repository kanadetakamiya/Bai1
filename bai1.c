#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD_LEN 100
#define MAX_STOPWORDS 1000

typedef struct LineNode
{
    int line;
    int count;
    struct LineNode *next;
} LineNode;

typedef struct BSTNode
{
    char word[MAX_WORD_LEN];
    LineNode *lines;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

typedef struct CapitalWord
{
    char word[MAX_WORD_LEN];
    LineNode *lines;
    struct CapitalWord *next;
} CapitalWord;

// Thêm dòng vào linked list theo thứ tự tăng dần, tăng count nếu dòng đã có
void add_line(LineNode **head, int line)
{
    LineNode *prev = NULL;
    LineNode *cur = *head;

    while (cur && cur->line < line)
    {
        prev = cur;
        cur = cur->next;
    }

    if (cur && cur->line == line)
    {
        cur->count++;
        return;
    }

    LineNode *node = (LineNode *)malloc(sizeof(LineNode));
    node->line = line;
    node->count = 1;
    node->next = cur;

    if (prev)
        prev->next = node;
    else
        *head = node;
}

// Thêm từ vào BST
BSTNode *add_bst(BSTNode *root, char *word, int line)
{
    if (!root)
    {
        BSTNode *node = (BSTNode *)malloc(sizeof(BSTNode));
        strcpy(node->word, word);
        node->lines = NULL;
        add_line(&(node->lines), line);
        node->left = node->right = NULL;
        return node;
    }
    int cmp = strcmp(word, root->word);
    if (cmp < 0)
        root->left = add_bst(root->left, word, line);
    else if (cmp > 0)
        root->right = add_bst(root->right, word, line);
    else
        add_line(&(root->lines), line);
    return root;
}

// Tính tổng số lần xuất hiện của từ
int total_count(LineNode *head)
{
    int sum = 0;
    while (head)
    {
        sum += head->count;
        head = head->next;
    }
    return sum;
}

// In BST vào file theo format: từ TỔNG_SỐ_LẦN dòng1 dòng2 ...
void print_bst(BSTNode *root, FILE *fout)
{
    if (!root)
        return;
    print_bst(root->left, fout);

    fprintf(fout, "%s %d", root->word, total_count(root->lines));
    LineNode *cur = root->lines;
    while (cur)
    {
        fprintf(fout, " %d", cur->line);
        cur = cur->next;
    }
    fprintf(fout, "\n");

    print_bst(root->right, fout);
}

// Binary search stopwords (đã viết thường, sắp xếp)
int binary_search_stopword(char **stopwords, int n, char *word)
{
    int l = 0, r = n - 1;
    while (l <= r)
    {
        int m = (l + r) / 2;
        int cmp = strcmp(word, stopwords[m]);
        if (cmp == 0)
            return 1;
        else if (cmp < 0)
            r = m - 1;
        else
            l = m + 1;
    }
    return 0;
}

// Thêm từ viết hoa vào danh sách tạm
CapitalWord *add_capital_word(CapitalWord *head, char *word, int line)
{
    CapitalWord *cur = head;
    while (cur)
    {
        if (strcmp(cur->word, word) == 0)
        {
            add_line(&(cur->lines), line);
            return head;
        }
        cur = cur->next;
    }
    CapitalWord *node = (CapitalWord *)malloc(sizeof(CapitalWord));
    strcpy(node->word, word);
    node->lines = NULL;
    add_line(&(node->lines), line);
    node->next = head;
    return node;
}

// Kiểm tra từ viết hoa có tồn tại dạng viết thường trong BST
int exists_in_bst(BSTNode *root, char *word)
{
    if (!root)
        return 0;
    int cmp = strcmp(word, root->word);
    if (cmp == 0)
        return 1;
    else if (cmp < 0)
        return exists_in_bst(root->left, word);
    else
        return exists_in_bst(root->right, word);
}

int main()
{
    FILE *ftext = fopen("vanban.txt", "r");
    FILE *fstop = fopen("stopw.txt", "r");
    if (!ftext || !fstop)
    {
        printf("Khong mo duoc file!\n");
        return 1;
    }

    // 1. Đọc stopwords
    char *stopwords[MAX_STOPWORDS];
    char temp[MAX_WORD_LEN];
    int stop_count = 0;
    while (fscanf(fstop, "%s", temp) == 1)
    {
        stopwords[stop_count] = (char *)malloc(strlen(temp) + 1);
        strcpy(stopwords[stop_count], temp);
        stop_count++;
    }
    fclose(fstop);

    BSTNode *bst = NULL;
    CapitalWord *capital_list = NULL;
    int line_num = 1;
    char c, word[MAX_WORD_LEN];
    int idx = 0;

    // 2. Đọc văn bản và tách từ
    while ((c = fgetc(ftext)) != EOF)
    {
        if (c == '\n')
            line_num++;
        if (isalpha(c))
        {
            word[idx++] = c;
        }
        else
        {
            if (idx > 0)
            {
                word[idx] = '\0';
                idx = 0;

                // Chuyển sang lowercase để kiểm tra stopword
                char lower[MAX_WORD_LEN];
                for (int i = 0; word[i]; i++)
                    lower[i] = tolower(word[i]);
                lower[strlen(word)] = '\0';

                // Lọc stopword ngay
                if (binary_search_stopword(stopwords, stop_count, lower))
                    continue;

                // Phân loại viết hoa/viết thường
                if (islower(word[0]))
                    bst = add_bst(bst, lower, line_num);
                else
                    capital_list = add_capital_word(capital_list, word, line_num);
            }
        }
    }
    fclose(ftext);

    // 3. Xử lý từ viết hoa
    CapitalWord *cur = capital_list;
    while (cur)
    {
        char lower[MAX_WORD_LEN];
        for (int i = 0; cur->word[i]; i++)
            lower[i] = tolower(cur->word[i]);
        lower[strlen(cur->word)] = '\0';

        if (exists_in_bst(bst, lower))
        {
            LineNode *ln = cur->lines;
            while (ln)
            {
                bst = add_bst(bst, lower, ln->line);
                ln = ln->next;
            }
        }
        cur = cur->next;
    }

    // 4. Xuất kết quả ra file output.txt
    FILE *fout = fopen("output.txt", "w");
    if (!fout)
    {
        printf("Khong mo duoc file output.txt!\n");
        return 1;
    }

    print_bst(bst, fout);
    fclose(fout);

    // Giải phóng stopwords
    for (int i = 0; i < stop_count; i++)
        free(stopwords[i]);
    return 0;
}
