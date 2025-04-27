#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BCD_BYTES 5
#define NEGATIVE_PREFIX 0xF
#define MAX_ADD_SUBTRACT_DIGITS 8
#define MAX_MULTIPLY_DIGITS 4

void print_bcd_bin(unsigned char *bcd);
void print_bcd_hex(unsigned char *bcd);
void int_to_bcd(int num, unsigned char *result);
unsigned char *bcd_add(unsigned char *a, unsigned char *b);
unsigned char *bcd_subtract(unsigned char *a, unsigned char *b);
unsigned char *bcd_multiply(unsigned char *a, unsigned char *b);
int bcd_compare(unsigned char *a, unsigned char *b);
unsigned char *complement_to_10(unsigned char *bcd);
int is_negative(unsigned char *bcd);
void set_negative(unsigned char *bcd);
int count_digits(unsigned char *bcd);
int validate_for_add_subtract(unsigned char *a, unsigned char *b);
int validate_for_multiply(unsigned char *a, unsigned char *b);

void bitwise_full_adder_1bit(int a, int b, int cin, int *sum_out,
                             int *cout_out) {
  *sum_out = (a ^ b) ^ cin;
  *cout_out = (a & b) | (a & cin) | (b & cin);
}

int bitwise_add_4bit_binary(int nibble_a, int nibble_b, int cin,
                            int *cout_4bit) {
  int s0, s1, s2, s3;
  int c0, c1, c2, c3;
  nibble_a &= 0x0F;
  nibble_b &= 0x0F;
  cin &= 1;

  bitwise_full_adder_1bit(nibble_a & 1, nibble_b & 1, cin, &s0, &c0);
  bitwise_full_adder_1bit((nibble_a >> 1) & 1, (nibble_b >> 1) & 1, c0, &s1,
                          &c1);
  bitwise_full_adder_1bit((nibble_a >> 2) & 1, (nibble_b >> 2) & 1, c1, &s2,
                          &c2);
  bitwise_full_adder_1bit((nibble_a >> 3) & 1, (nibble_b >> 3) & 1, c2, &s3,
                          &c3);

  int sum_4bit = (s3 << 3) | (s2 << 2) | (s1 << 1) | s0;
  *cout_4bit = c3;
  return sum_4bit;
}

int bitwise_add_bcd_nibble(int a_nib, int b_nib, int cin, int *cout_bcd) {
  int binary_sum_carry = 0;
  int binary_sum =
      bitwise_add_4bit_binary(a_nib, b_nib, cin, &binary_sum_carry);

  int s3 = (binary_sum >> 3) & 1;
  int s2 = (binary_sum >> 2) & 1;
  int s1 = (binary_sum >> 1) & 1;

  int correction_needed = binary_sum_carry | (s3 & (s2 | s1));
  int final_sum = binary_sum;
  int correction_carry = 0;

  if (correction_needed) {
    final_sum = bitwise_add_4bit_binary(binary_sum, 0x06, 0, &correction_carry);
  }

  *cout_bcd = binary_sum_carry | correction_needed;
  return final_sum & 0x0F;
}

int is_negative(unsigned char *bcd) { return (bcd[0] >> 4) == NEGATIVE_PREFIX; }

void set_negative(unsigned char *bcd) {
  bcd[0] = (NEGATIVE_PREFIX << 4) | (bcd[0] & 0x0F);
}

void int_to_bcd(int num, unsigned char *result) {
  memset(result, 0, MAX_BCD_BYTES);

  if (num > 99999999 || num < -99999999) {
    printf("Error: Number must be between -99999999 and 99999999\n");
    return;
  }

  int is_neg = num < 0;
  if (is_neg)
    num = -num;

  int idx = MAX_BCD_BYTES - 1;
  while (num > 0 && idx >= 0) {
    int digit1 = num % 10;
    num /= 10;

    int digit2 = 0;
    if (num > 0) {
      digit2 = num % 10;
      num /= 10;
    }

    result[idx] = ((digit2 & 0x0F) << 4) | (digit1 & 0x0F);
    idx--;
  }

  if (is_neg) {
    set_negative(result);
  }
}

unsigned char *complement_to_10(unsigned char *bcd) {
  unsigned char *result = (unsigned char *)malloc(MAX_BCD_BYTES);
  memset(result, 0, MAX_BCD_BYTES);

  unsigned char *bcdcopy = (unsigned char *)malloc(MAX_BCD_BYTES);
  memcpy(bcdcopy, bcd, MAX_BCD_BYTES);

  // First create 9's complement
  for (int i = 0; i < MAX_BCD_BYTES; i++) {
    // Skip the negative prefix if present
    unsigned char high =
        (i == 0 && is_negative(bcdcopy)) ? 0 : 9 - ((bcdcopy[i] >> 4) & 0x0F);
        unsigned char low = 9 - (bcdcopy[i] & 0x0F);
    result[i] = (high << 4) | low;
  }

  free(bcdcopy);

  // Add 1 to get 10's complement
  int carry = 1;
  for (int i = MAX_BCD_BYTES - 1; i >= 0; i--) {
    int low = (result[i] & 0x0F) + carry;
    carry = low > 9 ? 1 : 0;
    if (carry)
      low -= 10;

    int high = ((result[i] >> 4) & 0x0F);
    if (carry) {
      high++;
      carry = high > 9 ? 1 : 0;
      if (carry)
        high -= 10;
    }

    result[i] = (high << 4) | low;
  }

  return result;
}

unsigned char *bcd_add(unsigned char *a, unsigned char *b) {
  if (!validate_for_add_subtract(a, b)) {
    return NULL;
  }

  unsigned char *result = (unsigned char *)malloc(MAX_BCD_BYTES);
  memset(result, 0, MAX_BCD_BYTES);

  int a_neg = is_negative(a);
  int b_neg = is_negative(b);

  // Case 1: A + B (both positive)
  if (!a_neg && !b_neg) {
    int carry = 0;
    for (int i = MAX_BCD_BYTES - 1; i >= 0; i--) {
      int a_low = a[i] & 0x0F;
      int b_low = b[i] & 0x0F;
      int a_high = (a[i] >> 4) & 0x0F;
      int b_high = (b[i] >> 4) & 0x0F;

      int sum_low = 0, carry_low = 0;
      int sum_high = 0, carry_high = 0;

      sum_low = bitwise_add_bcd_nibble(a_low, b_low, carry, &carry_low);
      sum_high = bitwise_add_bcd_nibble(a_high, b_high, carry_low, &carry_high);

      result[i] = (sum_high << 4) | sum_low;
      carry = carry_high;
    }
  }
  // Case 2: -A + B = B - A
  else if (a_neg && !b_neg) {
    unsigned char *a_complement = complement_to_10(a);
    result = bcd_add(a_complement, b);

  }
  // Case 3: A + (-B) = A - B
  else if (!a_neg && b_neg) {
    unsigned char *b_complement = complement_to_10(b); //doesnt work
    // -8 -> 1001 1001 1001 1001 1001 1001 1001 1001 0010
    print_bcd_bin(b_complement); // TODO: remove (debugging)
    result = bcd_add(a, b_complement);
  }
  // Case 4: -A + (-B) = -(A + B)
  else {
    unsigned char *pos_a = (unsigned char *)malloc(MAX_BCD_BYTES);
    unsigned char *pos_b = (unsigned char *)malloc(MAX_BCD_BYTES);
    memcpy(pos_a, a, MAX_BCD_BYTES);
    memcpy(pos_b, b, MAX_BCD_BYTES);
    pos_a[0] &= 0x0F;
    pos_b[0] &= 0x0F;
    result = bcd_add(pos_a, pos_b);
    set_negative(result);
    free(pos_a);
    free(pos_b);
  }

  return result;
}

int count_digits(unsigned char *bcd) {
  int digits = 0;
  int started = 0;
  int is_neg = is_negative(bcd);

  for (int i = 0; i < MAX_BCD_BYTES; i++) {
    unsigned char high = (bcd[i] >> 4) & 0x0F;
    unsigned char low = bcd[i] & 0x0F;

    // Skip negative flag for first byte
    if (i == 0 && is_neg) {
      if (low != 0) {
        digits++;
        started = 1;
      }
      continue;
    }

    // Count high nibble if non-zero or we've started counting
    if (high != 0 || started) {
      digits++;
      started = 1;
    }

    // Count low nibble if non-zero or we've started counting
    if (low != 0 || started) {
      digits++;
      started = 1;
    }
  }

  return digits == 0 ? 1 : digits;
}

int validate_for_add_subtract(unsigned char *a, unsigned char *b) {
  int a_digits = count_digits(a);
  int b_digits = count_digits(b);

  if (a_digits > MAX_ADD_SUBTRACT_DIGITS ||
      b_digits > MAX_ADD_SUBTRACT_DIGITS) {
    printf(
        "Error: Numbers must not exceed %d digits for addition/subtraction\n",
        MAX_ADD_SUBTRACT_DIGITS);
    return 0;
  }
  return 1;
}

int validate_for_multiply(unsigned char *a, unsigned char *b) {
  int a_digits = count_digits(a);
  int b_digits = count_digits(b);

  if (a_digits > MAX_MULTIPLY_DIGITS || b_digits > MAX_MULTIPLY_DIGITS) {
    printf("Error: Numbers must not exceed %d digits for multiplication\n",
           MAX_MULTIPLY_DIGITS);
    return 0;
  }
  return 1;
}

unsigned char *bcd_subtract(unsigned char *a, unsigned char *b) {
  if (!validate_for_add_subtract(a, b)) {
    return NULL;
  }

  unsigned char *result = (unsigned char *)malloc(MAX_BCD_BYTES);
  memset(result, 0, MAX_BCD_BYTES);

  int a_neg = is_negative(a);
  int b_neg = is_negative(b);

  // Case 1: A - B = A + (-B)
  if (!a_neg && !b_neg) {
    unsigned char *neg_b = (unsigned char *)malloc(MAX_BCD_BYTES);
    memcpy(neg_b, b, MAX_BCD_BYTES);
    set_negative(neg_b);
    result = bcd_add(a, neg_b);
    free(neg_b);
  }
  // Case 2: A - (-B) = A + B
  else if (!a_neg && b_neg) {
    unsigned char *pos_b = (unsigned char *)malloc(MAX_BCD_BYTES);
    memcpy(pos_b, b, MAX_BCD_BYTES);
    pos_b[0] &= 0x0F; // Clear negative flag
    result = bcd_add(a, pos_b);
    free(pos_b);
  }
  // Case 3: -A - B = -(A + B)
  else if (a_neg && !b_neg) {
    unsigned char *pos_a = (unsigned char *)malloc(MAX_BCD_BYTES);
    memcpy(pos_a, a, MAX_BCD_BYTES);
    pos_a[0] &= 0x0F; // Clear negative flag
    result = bcd_add(pos_a, b);
    set_negative(result);
    free(pos_a);
  }
  // Case 4: -A - (-B) = -A + B
  else {
    unsigned char *pos_b = (unsigned char *)malloc(MAX_BCD_BYTES);
    memcpy(pos_b, b, MAX_BCD_BYTES);
    pos_b[0] &= 0x0F;
    result = bcd_add(a, pos_b);
    free(pos_b);
  }

  return result;
}

unsigned char *bcd_multiply(unsigned char *a, unsigned char *b) {
  if (!validate_for_multiply(a, b)) {
    return NULL;
  }

  unsigned char *result = (unsigned char *)malloc(MAX_BCD_BYTES);
  memset(result, 0, MAX_BCD_BYTES);

  int a_neg = is_negative(a);
  int b_neg = is_negative(b);

  // Create positive copies of inputs
  unsigned char *pos_a = (unsigned char *)malloc(MAX_BCD_BYTES);
  unsigned char *pos_b = (unsigned char *)malloc(MAX_BCD_BYTES);
  memcpy(pos_a, a, MAX_BCD_BYTES);
  memcpy(pos_b, b, MAX_BCD_BYTES);

  // Clear negative flags if present
  if (a_neg)
    pos_a[0] &= 0x0F;
  if (b_neg)
    pos_b[0] &= 0x0F;

  // For each digit in b
  for (int i = MAX_BCD_BYTES - 1; i >= 0; i--) {
    // Process each digit in current byte of b
    for (int digit = 0; digit < 2; digit++) {
      unsigned char b_digit =
          (digit == 0) ? (pos_b[i] & 0x0F) : ((pos_b[i] >> 4) & 0x0F);
      if (b_digit == 0)
        continue;

      unsigned char *partial = (unsigned char *)malloc(MAX_BCD_BYTES);
      memset(partial, 0, MAX_BCD_BYTES);

      // Calculate position shift for this digit of b
      int shift_amount = ((MAX_BCD_BYTES - 1 - i) * 2 + digit);

      // Multiply each digit of a by current digit of b
      int carry = 0;
      for (int j = MAX_BCD_BYTES - 1; j >= 0; j--) {
        for (int k = 0; k < 2; k++) {
          unsigned char a_digit =
              (k == 0) ? (pos_a[j] & 0x0F) : ((pos_a[j] >> 4) & 0x0F);

          // Calculate product and add carry
          int prod = (a_digit * b_digit) + carry;
          carry = prod / 10;
          prod %= 10;

          // Calculate position for this digit in result
          int pos_shift = ((MAX_BCD_BYTES - 1 - j) * 2 + k + shift_amount);
          int byte_pos = MAX_BCD_BYTES - 1 - (pos_shift / 2);
          int nibble_pos = pos_shift % 2;

          if (byte_pos >= 0) {
            if (nibble_pos == 0) {
              partial[byte_pos] |= prod;
            } else {
              partial[byte_pos] |= (prod << 4);
            }
          }
        }
      }

      // Add partial product to result
      unsigned char *new_result = bcd_add(result, partial);
      free(result);
      free(partial);
      result = new_result;
    }
  }

  // Set sign of result
  if (a_neg ^ b_neg) {
    set_negative(result);
  }

  free(pos_a);
  free(pos_b);
  return result;
}

int bcd_compare(unsigned char *a, unsigned char *b) {
  int a_neg = is_negative(a);
  int b_neg = is_negative(b);

  // Different signs
  if (a_neg && !b_neg)
    return -1;
  if (!a_neg && b_neg)
    return 1;

  // Same signs
  int multiplier = a_neg ? -1 : 1;

  for (int i = 0; i < MAX_BCD_BYTES; i++) {
    unsigned char a_val = a[i] & (i == 0 ? 0x0F : 0xFF);
    unsigned char b_val = b[i] & (i == 0 ? 0x0F : 0xFF);

    if (a_val > b_val)
      return 1 * multiplier;
    if (a_val < b_val)
      return -1 * multiplier;
  }

  return 0;
}

void print_bcd_bin(unsigned char *bcd) {
  printf("Binary: ");

  // Handle negative numbers
  int is_neg = is_negative(bcd);
  if (is_neg) {
    printf("1111 ");
  }

  // Find first significant digit (skipping the negative flag if present)
  int start_idx = 0;
  int found = 0;

  // First, find the first non-zero byte
  for (int i = 0; i < MAX_BCD_BYTES; i++) {
    unsigned char byte = bcd[i];
    if (i == 0 && is_neg) {
      byte &= 0x0F; // Clear the negative flag for checking
    }

    // Check both nibbles
    unsigned char high = (byte >> 4) & 0x0F;
    unsigned char low = byte & 0x0F;

    if (high != 0 || low != 0) {
      start_idx = i;
      found = 1;
      break;
    }
  }

  // If number is zero
  if (!found) {
    printf("0000");
    printf("\n");
    return;
  }

  // Print significant digits
  int first_nibble = 1;
  for (int i = start_idx; i < MAX_BCD_BYTES; i++) {
    unsigned char high = (bcd[i] >> 4) & 0x0F;
    unsigned char low = bcd[i] & 0x0F;

    // Handle first byte for negative numbers
    if (i == 0 && is_neg) {
      if (low != 0 || !first_nibble) {
        for (int j = 3; j >= 0; j--) {
          printf("%d", (bcd[i] >> j) & 1);
        }
        first_nibble = 0;
      }
      continue;
    }

    // For regular bytes
    if (first_nibble) {
      // For the first non-zero nibble, we need to handle leading zeros
      if (high != 0) {
        // Print high nibble with its proper zeros
        for (int j = 7; j >= 4; j--) {
          printf("%d", (bcd[i] >> j) & 1);
        }
        printf(" ");
        first_nibble = 0;
      }
    } else {
      // After first nibble, print all nibbles with proper spacing
      for (int j = 7; j >= 4; j--) {
        printf("%d", (bcd[i] >> j) & 1);
      }
      printf(" ");
    }

    // Always print low nibble if we've printed high nibble or if it's non-zero
    if (!first_nibble || low != 0) {
      for (int j = 3; j >= 0; j--) {
        printf("%d", (bcd[i] >> j) & 1);
      }
      if (i < MAX_BCD_BYTES - 1)
        printf(" ");
      first_nibble = 0;
    }
  }
  printf("\n");
}

void print_bcd_hex(unsigned char *bcd) {
  printf("Hex: ");

  // Handle negative numbers
  int is_neg = is_negative(bcd);
  if (is_neg) {
    printf("-");
  }

  // Find first significant digit (skipping the negative flag if present)
  int start_idx = 0;
  int found = 0;

  // First, find the first non-zero byte
  for (int i = 0; i < MAX_BCD_BYTES; i++) {
    unsigned char byte = bcd[i];
    if (i == 0 && is_neg) {
      byte &= 0x0F; // Clear the negative flag for checking
    }

    // Check both nibbles
    unsigned char high = (byte >> 4) & 0x0F;
    unsigned char low = byte & 0x0F;

    if (high != 0 || low != 0) {
      start_idx = i;
      found = 1;
      break;
    }
  }

  // If number is zero
  if (!found) {
    printf("00");
    printf("\n");
    return;
  }

  // Print significant digits
  int first_byte = 1;
  for (int i = start_idx; i < MAX_BCD_BYTES; i++) {
    if (i == 0 && is_neg) {
      // Only print low nibble for first byte if negative
      unsigned char low = bcd[i] & 0x0F;
      if (low != 0 || first_byte) {
        printf("%01X", low);
        first_byte = 0;
      }
    } else {
      // For non-first bytes or positive numbers
      unsigned char byte = bcd[i];
      if (byte != 0 || !first_byte) {
        if (!first_byte)
          printf(" ");
        printf("%02X", byte);
        first_byte = 0;
      }
    }
  }
  printf("\n");
}

void Menu() {
  printf("\nMenu:\n");
  printf("1. Enter the first number\n");
  printf("2. Enter the second number\n");
  printf("3. Add\n");
  printf("4. Subtract\n");
  printf("5. Multiply\n");
  printf("6. Compare\n");
  printf("7. End\n");
  printf("Choice: ");
}

int main() {
  int choice = 0;
  int num1 = 0, num2 = 0;
  unsigned char *operand1 = (unsigned char *)malloc(MAX_BCD_BYTES);
  unsigned char *operand2 = (unsigned char *)malloc(MAX_BCD_BYTES);
  unsigned char *result;
  memset(operand1, 0, MAX_BCD_BYTES);
  memset(operand2, 0, MAX_BCD_BYTES);

  while (1) {
    Menu();
    scanf("%d", &choice);

    switch (choice) {
    case 1:
      printf("Enter the first number: ");
      scanf("%d", &num1);
      int_to_bcd(num1, operand1);
      print_bcd_bin(operand1);
      print_bcd_hex(operand1);
      break;

    case 2:
      printf("Enter the second number: ");
      scanf("%d", &num2);
      int_to_bcd(num2, operand2);
      print_bcd_bin(operand2);
      print_bcd_hex(operand2);
      break;

    case 3:
      printf("Adding...\n");
      result = bcd_add(operand1, operand2);
      if (result) {
        print_bcd_bin(result);
        print_bcd_hex(result);
        printf("Check: %d + %d = %d\n", num1, num2, num1 + num2);
        free(result);
      }
      break;

    case 4:
      printf("Subtracting...\n");
      result = bcd_subtract(operand1, operand2);
      if (result) {
        print_bcd_bin(result);
        print_bcd_hex(result);
        printf("Check: %d - %d = %d\n", num1, num2, num1 - num2);
        free(result);
      }
      break;

    case 5:
      printf("Multiplying...\n");
      result = bcd_multiply(operand1, operand2);
      if (result) {
        print_bcd_bin(result);
        print_bcd_hex(result);
        printf("Check: %d * %d = %d\n", num1, num2, num1 * num2);
        free(result);
      }
      break;

    case 6:
      printf("Comparing...\n");
      int cmp = bcd_compare(operand1, operand2);
      if (cmp > 0)
        printf("First number is larger\n");
      else if (cmp < 0)
        printf("Second number is larger\n");
      else
        printf("Numbers are equal\n");
      printf("Check: %d %c %d\n", num1, cmp > 0 ? '>' : (cmp < 0 ? '<' : '='),
             num2);
      break;

    case 7:
      printf("Ending program...\n");
      free(operand1);
      free(operand2);
      return 0;

    default:
      printf("Invalid choice!\n");
    }
  }

  return 0;
}
