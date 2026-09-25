// Quick verification program to check PDO sizes
#include <stdio.h>
#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    uint16_t control_word;
    int32_t  target_position;
    uint16_t touch_probe_function;
    int32_t  padding1;
    int32_t  padding2;
    int32_t  padding3;
    int32_t  padding4;
    uint16_t padding5;
} OutputPDO;

typedef struct {
    uint16_t error_code;
    uint16_t status_word;
    int32_t  position_actual_value;
    int16_t  torque_actual_value;
    int32_t  position_deviation;
    uint16_t touch_probe_status;
    int32_t  touch_probe_pos1_value;
    int32_t  touch_probe_pos2_value;
} InputPDO;

#pragma pack(pop)

int main() {
    printf("PDO Structure Verification - Hardware Mapping 0x1702/0x1B02\n");
    printf("============================================================\n\n");
    
    printf("OutputPDO (RxPDO 0x1702):\n");
    printf("  Expected size: 26 bytes\n");
    printf("  Actual size:   %zu bytes\n", sizeof(OutputPDO));
    printf("  Status: %s\n\n", (sizeof(OutputPDO) == 26) ? "✓ PASS" : "✗ FAIL");
    
    printf("InputPDO (TxPDO 0x1B02):\n");
    printf("  Expected size: 24 bytes\n");
    printf("  Actual size:   %zu bytes\n", sizeof(InputPDO));
    printf("  Status: %s\n\n", (sizeof(InputPDO) == 24) ? "✓ PASS" : "✗ FAIL");
    
    if (sizeof(OutputPDO) == 26 && sizeof(InputPDO) == 24) {
        printf("Overall: ✓ ALL CHECKS PASSED\n");
        printf("\nThis matches the hardware mapping detected by --check-pdo\n");
        return 0;
    } else {
        printf("Overall: ✗ SOME CHECKS FAILED\n");
        return 1;
    }
}
