#define FW_OFFSET 0x31000
#define RESET_VECTOR_OFFSET 0x4

void _start(void)
{
	void (* jump)(void);
	jump = (void *)(*(unsigned int *)(FW_OFFSET + RESET_VECTOR_OFFSET));

	jump();

	while (1);
}
