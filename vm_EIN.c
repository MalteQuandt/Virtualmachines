// simple virtual machine, which implements the assembly language that i learned
// in EIN at the FH kiel.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// prototypes for the functions.
int executeInstruction();
int isolateOperation(uint32_t);
int isolateOperand(uint32_t);
int isolateAddress(uint32_t);
int initializeMemory(char *);
void removeSpaces(char *);
int32_t parseLine(char *);
int getInstruction(char *);
void parseInstructions(char *);
int getOperation(char *, char *);
int getOperand(char *, char *);
int getAddress(char *, char *);
void removeComments(char *);
void removeNewLine(char *);
long getFileLength(FILE *);

// Offset for the data in the array.
#define DATA_OFFSET (2 << 11)

// For debuggin purposes
#define DEBUG 0
/* Instructions: */
enum {
  ADD = 0,
  SUB,
  MUL,
  DIV,
  LDA,
  LDK,
  STA,
  JMP,
  JEZ,
  JNE,
  JLZ,
  JLE,
  JGZ,
  JGE,
  INP,
  OUT,
  HLT /* = 16, so we need 5 bits to represent the instructions! */
};
/* 65536 dates with 32 bits of memory each */
static uint32_t memory[UINT16_MAX + (2 << 11)];
/* Registers: */
static uint32_t PC = 1; // The program flow starts at the address one!
static uint32_t OPC;    // The current instruction.
static int32_t AKK;     // Akkumulator

int main(int argc, char *argv[]) {
  if (argc > 2) {
    // TODO:
    // Read code from file, read code from scanline, MAYBE pad addresses and
    // operands to the correct length.
    printf("The program has to many arguments!");
    return 1;
  } else {

    // Test instructions, which do nothing except print '6' "0001 LDK
    // 00006\n0002 STA 00001\n0003 OUT 00001\n0004 HLT 00099"

    // Everything is fine, continue with the execution:
    // Initialize memory:
    parseInstructions(argv[1]);
    // Main loop of the vm.
    while (1) {
      executeInstruction();
    }
  }
  system("pause");
  return EXIT_SUCCESS;
}

// Executes the current instruction and
int executeInstruction() {
  // 1. Befehl holen
  OPC = memory[PC];
  // 2. Operand holen
  int operand = isolateOperand(OPC), operation = isolateOperation(OPC);
  // 3. Befehl dekodieren & Befehl ausfuehren
  switch (operation) {
  case ADD:
    AKK += memory[operand + DATA_OFFSET];
    break;
  case SUB:
    AKK -= memory[operand + DATA_OFFSET];
    break;
  case MUL:
    AKK *= memory[operand + DATA_OFFSET];
    break;
  case DIV:
    AKK /= memory[operand + DATA_OFFSET];
    break;
  case LDA:
    AKK = memory[operand + DATA_OFFSET];
    break;
  case LDK:
    AKK = operand;
    break;
  case STA:
    memory[operand + DATA_OFFSET] = AKK;
    break;
  case JMP:
    PC = operand;
    return EXIT_SUCCESS;
  case JEZ:
    if (AKK == 0) {
      PC = operand;
    }
    return EXIT_SUCCESS;
  case JNE:
    if (AKK != 0) {
      PC = operand;
    }
    return EXIT_SUCCESS;
  case JLZ:
    if (AKK < 0) {
      PC = operand;
    }
    return EXIT_SUCCESS;
  case JLE:
    if (AKK <= 0) {
      PC = operand;
    }
    return EXIT_SUCCESS;
  case JGZ:
    if (AKK > 0) {
      PC = operand;
    }
    return EXIT_SUCCESS;
  case JGE:
    if (AKK >= 0) {
      PC = operand;
    }
    return EXIT_SUCCESS;
  case INP:
    printf("\nPlease input some number: ");
    scanf("%d", memory + operand + DATA_OFFSET);
    break;
  case OUT:
    printf("\n%d", memory[operand + DATA_OFFSET]);
    break;
  case HLT:
    /* trap signal */
    switch (operand) {
    case 99:
      exit(0);
      break;
      // There is space for up to 65534 more codes.
    default:
      printf("\nThis trap signal does not exit!\n");
      break;
    }
  default:
    printf("\nThis instruction does not exist!\n");
    exit(0);
  }

  /* Increase the program counter: */
  // 5. Befehlszaeler aendern
  PC++;
  return EXIT_SUCCESS;
}
inline int isolateOperation(uint32_t instruction) {
  return (instruction >> 16) & 0x1F;
}
inline int isolateOperand(uint32_t instruction) {
  return (instruction & 0xFFFF);
}
inline int isolateAddress(uint32_t instruction) {
  return (instruction >> 21) & 0x7FF;
}

void removeNewLine(char *s) {
  const char *d = s;
  do {
    while (*d == '\n') {
      ++d;
    }
  } while ((*s++ = *d++));
}

void removeSpaces(char *s) {
  const char *d = s;
  do {
    while (*d == ' ') {
      ++d;
    }
  } while ((*s++ = *d++));
}

int32_t parseLine(char *string) {
  int32_t instruction, address, operand;
  char buffer[6];
#if DEBUG
  printf("String: %s\n", string);
#endif
  instruction = getOperation(buffer, string);
#if DEBUG
  printf("Instruction: %d\n", instruction);
#endif
  // Get the address:
  address = getAddress(buffer, string);
#if DEBUG
  printf("Address: %d\n", address);
#endif
  // Get the operand:
  operand = getOperand(buffer, string);
#if DEBUG
  printf("Operand: %d\n", operand);
#endif
  return operand + (address << 21) + (instruction << 16);
}

int getOperation(char *buffer, char *string) {
  memcpy(buffer, string + 4, 3);
  buffer[3] = '\0';
  return getInstruction(buffer);
}
int getOperand(char *buffer, char *string) {
  memcpy(buffer, string + 7, 5);
  buffer[6] = '\0';
  return atoi(buffer);
}
int getAddress(char *buffer, char *string) {
  memcpy(buffer, string, 4);
  buffer[4] = '\0';
  return atoi(buffer);
}

void parseInstructions(char *string) {

  removeSpaces(string);
  removeNewLine(string);

  int len = strlen(string);
  char buffer[13];
  buffer[12] = '\0';
  if (len % 12 != 0) {
    return;
  } else {
    for (int i = 0, j = 0; i < len; i += 12, j++) {
      memcpy(buffer, string + i, 12);
      memory[j + 1] = parseLine(buffer);
    }
  }
}

/**************************************
 * Name: getInstruction
 * Beschreibung: Returns the instruction integer which is assigned to the string
 * in the argument Formale Parameter:
 * string Rueckgabewert: instruction
 * Version: 1.0
 * Author: Malte Quandt
 * Datum: 2021-02-11
 **************************************/
int getInstruction(char *string) {
  if (strcmp(string, "ADD") == 0) {
    return ADD;
  } else if (strcmp(string, "SUB") == 0) {
    return SUB;
  } else if (strcmp(string, "MUL") == 0) {
    return MUL;
  } else if (strcmp(string, "DIV") == 0) {
    return DIV;
  } else if (strcmp(string, "INP") == 0) {
    return INP;
  } else if (strcmp(string, "OUT") == 0) {
    return OUT;
  } else if (strcmp(string, "HLT") == 0) {
    return HLT;
  } else if (strcmp(string, "LDA") == 0) {
    return LDA;
  } else if (strcmp(string, "LDK") == 0) {
    return LDK;
  } else if (strcmp(string, "STA") == 0) {
    return STA;
  } else if (strcmp(string, "JMP") == 0) {
    return JMP;
  } else if (strcmp(string, "JEZ") == 0) {
    return JEZ;
  } else if (strcmp(string, "JNE") == 0) {
    return JNE;
  } else if (strcmp(string, "JLZ") == 0) {
    return JLZ;
  } else if (strcmp(string, "JLE") == 0) {
    return JLE;
  } else if (strcmp(string, "LGZ") == 0) {
    return JGZ;
  } else if (strcmp(string, "JGE") == 0) {
    return JGE;
  } else {
    return -1;
  }
}

/**************************************
 * Name: removeComments
 * Beschreibung: Removes all the comments which start with ';' and are ended by
 *either ';', '\n', '\0'. Formale Parameter: assembler program Rueckgabewert:
 * Version:
 * Author: Malte Quandt
 * Datum: 2021-02-11
 **************************************/
void removeComments(char *string) {
  // the length of the argument string.
  int len = strlen(string) + 1;
  for (int i = 0; i < len && string[i] != '\0'; i++) {
    // if the element at i is a ';' character, iterate until either another ';'
    // or '\0' or '\n' is found, and if it is, replace all of its contents with
    // spaces.
    if (string[i] == ';') {
      // Replace the ';' with a space.
      string[i] = ' ';
      // Iterate over the rest of the string until the break-character is found.
      for (int j = 1; j + i < len; j++) {
        if (string[i + j] == ';') {
          // Replace the contents plus the ';' character and break the loop.
          string[i + j] = ' ';
          break;
        } else if (string[i + j] == '\0' || string[i + j] == '\n') {
          // Break the loop.
          break;
        } else {
          // Replace the character with a space and continue.
          string[i + j] = ' ';
          continue;
        }
      }
    }
  }
}

/**************************************
 * Name: getFileLength
 * Beschreibung:  Return the size of the file in the parameter.
 * Formale Parameter: file
 * Rueckgabewert: length
 * Version: 1.0
 * Author: Malte Quandt
 * Datum: 2021-02-12
 **************************************/
long getFileLength(FILE *file) {
  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);
  return length;
}
/**************************************
 * Name: readFile 
 * Beschreibung: Reads the data of the file, and returns it as a character buffer. 
 * Formale Parameter: path: path to the file 
 * Rueckgabewert: buffer: buffer with the file info 
 * Version: 1.0 
 * Author: Malte Quandt 
 * Datum: 2021-02-12  
 **************************************/
char *readFile(char *path) {
  char *buffer = NULL;
  long length = 0l;
  FILE *file = fopen(path, "rb");
  if (file) {
    length = getFileLength(file);
    buffer = malloc(length * sizeof(char));
    // If the malloc did not fail, read the info into the string buffer.
    if (buffer) {
      read(buffer, 1, length, file);
    }
    // Stream is no longer needed, but the operating system might need it later,
    // thus we need to close it.
    fclose(file);
  }
  return buffer;
}

