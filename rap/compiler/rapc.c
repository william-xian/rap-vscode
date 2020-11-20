
#include <deed.h>
#include <rap_hash.h>
#include <rap_array.h>
#include <rapc.h>

#define VECTOR_SIZE 3 * 100
#define PATTERN_SIZE 20


rap_pool_t *POOL;

enum range_type_t
{
	NONE,
	STRING,
	CLASS,
	DEED,
	FIELD,
	FUNCTION,
	INT,
	INT2,
	INT4,
	INT8,
};

typedef struct
{
	ushort start;
	ushort end;
} range_t;

def_a_(range_t)

typedef struct
{
	short class_no;
	range_t content;
	range_t name;
} field_t;

typedef struct
{
	short class_no;
	range_t content;
	range_t name;
	a_range_t args;
	a_range_t returns;
	a_range_t expresses;
	/* data */
} function_t;

typedef struct
{
	range_t modifier;
	range_t content;
	range_t defGenericType;
	range_t supers[16];
} class_t;

typedef struct
{
	char filename[256];
	ushort filesize;
	char *data;
	char *content;
	ushort class_cnt;
	range_t package;
	a_range_t imports;
	a_range_t comments;
	a_range_t strings;
	class_t classes[32];
	field_t fields[128];
	function_t functions[512];
} source_t;

typedef struct {
	int type;
	const char *names;
} word_t;

typedef struct {
	int type;
	const char *names;
} expr_t;
enum token_t{
	SET,CMP,BIT,ARITHMETIC,LGC,BRANCH,LOOP
};

word_t words[] = {
	{SET, "="},
	{SET, "+="},
	{SET, "-="},
	{SET, "*="},
	{SET, "/="},
	{SET, "|="},
	{SET, "&="},
	{SET, "^="},
	{CMP, ">"},
	{CMP, "<"},
	{CMP, "=="},
	{CMP, ">="},
	{CMP, "<="},
	{BIT, "&"},
	{BIT, "|"},
	{BIT, "^"},
	{BIT, "<<"},
	{BIT, "<<<"},
	{BIT, ">>"},
	{BIT, ">>>"},
	{ARITHMETIC, "+"},
	{ARITHMETIC, "++"},
	{ARITHMETIC, "-"},
	{ARITHMETIC, "*"},
	{ARITHMETIC, "/"},
	{LGC, "&&"},
	{LGC, "||"},
	{LGC, "!"},
	{BRANCH, "if"},
	{BRANCH, "else"},
	{BRANCH, "else if"},
	{BRANCH, "switch"},
	{BRANCH, "case"},
	{BRANCH, "default"},
	{BRANCH, "break"},
	{BRANCH, "continue"},
	{LOOP, "for"},
	{LOOP, "while"},
	{LOOP, "do...while"},
	{0, ""}
};

enum pattern_name
{
	COMMENT,
	CONST_STRING,
	CONST_CHAR,
	PACKAGE,
	IMPORT,
	BACKET,
	COMMA
};

static char *patterns[PATTERN_SIZE] = {
	"/\\*(.|\n)*?\\*/",
	"\".*?\"",
	"'.*?'",
	"package .*;",
	"import .*;",
	"\\{(([^{}]*|(?R))*)\\}",
	"\\((([^()]*|(?R))*)\\)",
	NULL};

static pcre *pcres[PATTERN_SIZE];

int errno;
const char *error;

void printRange(char *str, ushort start, ushort end)
{
	char buf[8196];
	strncpy(buf, &str[start], end - start);
	buf[end - start] = '\0';
	printf("%s\n", buf);
}

void source_replace_string(char *str, int len)
{
	char before = ' ', open = ' ', cur;
	for (int i = 0; i < len; i++)
	{
		cur = str[i];
		if (open != ' ')
		{
			if (str[i] == open)
			{
				if (before != '\\')
				{
					open = ' ';
				}
				else
				{
					str[i] = '-';
				}
			}
			else
			{
				str[i] = '_';
			}
		}
		else
		{
			if (str[i] == '"' || str[i] == '\'')
			{
				open = str[i];
			}
		}
		before = cur;
	}
}

int source_parse(source_t *src)
{
	log_d("启动") int rc;
	int ovector[3];
	int start = 0;
	
	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[COMMENT], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			/* 删除注释 */
			range_t *val = rap_array_push(&src->comments);
			val->start = ovector[0];
			val->end = ovector[1];
			
			for (int i = ovector[0]; i < ovector[1]; i++)
			{
				src->content[i] = ' ';
			}
			start = ovector[1];
		}
		else
		{
			break;
		}
	}

	source_replace_string(src->content, src->filesize);
	start = 0;
	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[CONST_STRING], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			range_t *val = rap_array_push(&src->strings);
			val->start = ovector[0];
			val->end = ovector[1];
			start = ovector[1];
		}
		else
		{
			break;
		}
	}
	start = 0;
	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[CONST_CHAR], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			range_t *val = rap_array_push(&src->strings);
			val->start = ovector[0];
			val->end = ovector[1];
			start = ovector[1];
		}
		else
		{
			break;
		}
	}

	start = 0;
	while (start < src->filesize)
	{
		rc = pcre_exec(pcres[BACKET], NULL, src->content, src->filesize, start, 0, ovector, 3);
		if (rc >= 0)
		{
			printRange(src->content, ovector[0], ovector[1]);
			start = ovector[1];
		}
		else
		{
			break;
		}
	}
	return 0;
}

int source_load(source_t *src)
{
	FILE *fp = fopen(src->filename, "r");
	assert(fp != NULL);
	errno = fseek(fp, 0, SEEK_END);
	assert(errno == 0);
	int filesize = ftell(fp);
	errno = fseek(fp, 0, SEEK_SET);
	assert(errno == 0);
	src->filesize = filesize;
	src->data = malloc(filesize + 1);
	src->content = malloc(filesize + 1);
	fread(src->data, filesize, filesize, fp);
	src->data[filesize] = '\0';
	strncpy(src->content, src->data, filesize + 1);
	errno = fclose(fp);
	assert(errno == 0);
	return 0;
}

int source_init(source_t *src, char *filename)
{
	log_d("启动")
	range_t *pv;
	int i = 0;
	for (i = 0; patterns[i] != NULL; i++)
	{
		pcres[i] = pcre_compile(patterns[i], 0, &error, &errno, NULL);
		if (errno != 0)
		{
			log_e("pattern:%s, errno: %d, error: %s", patterns[i], errno, error);
		}
	}

	strcpy(src->filename, filename);
	src->filename[strlen(filename)] = '\0';
	source_load(src);

	rap_array_init(&src->imports, POOL, 8, sizeof(range_t));
	rap_array_init(&src->comments, POOL, 8, sizeof(range_t));
	rap_array_init(&src->strings, POOL, 8, sizeof(range_t));

	source_parse(src);

	for (i = 0; patterns[i] != NULL; i++)
	{
		pcre_free(pcres[i]);
	}

	//printf(">>>>>>>>>>> %s >>>>>>>>>>>\n%s\n<<<<<<<<<<< %s <<<<<<<<<<<\n", src->filename, src->content, src->filename);

	for (i = 0, pv = src->comments.elts; i < src->comments.nelts; i++,pv++)
	{	
		printRange(src->data, pv->start, pv->end);
	}
	for (i = 0, pv = src->strings.elts; i < src->strings.nelts; i++,pv++)
	{	
		printRange(src->data, pv->start, pv->end);
	}
	return 0;
}


void f() {
	rap_hash_keys_arrays_t ha;
	rap_hash_init_t classes;
	int v[3] = {1,2,3};
	rap_hash_key_t names = {};
	rap_hash_keys_array_init(&ha, 4);
	rap_hash_init(&classes,&names, 4);
	rap_str_t  key = rap_string("a");
	rap_hash_add_key(&ha, &key, &v[0], 0);
	rap_hash_find(classes.hash, rap_hash_key(key.data,key.len),key.data,key.len);
}

int main(int argc, char *argv[])
{
	log_d("启动")
	POOL = rap_create_pool(1024);
	source_t src;
	source_init(&src, argv[1]);
	rap_destroy_pool(POOL);
	return 0;
}


