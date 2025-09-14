/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "include/limine.h"

/* Put every request in this file, defined exactly once. */
__attribute__((
    used,
    section(".requests"))) volatile struct limine_memmap_request memmap_req = {
    .id       = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
};

__attribute__((
    used,
    section(".requests"))) volatile struct limine_hhdm_request hhdm_request = {
    .id       = LIMINE_HHDM_REQUEST,
    .revision = 0,
};

__attribute__((
    used, section(".requests"))) volatile struct limine_kernel_address_request
    Kaddress_req = {
        .id       = LIMINE_KERNEL_ADDRESS_REQUEST,
        .revision = 0,
};

__attribute__((
    used,
    section(
        ".requests"))) volatile struct limine_module_request module_request = {
    .id       = LIMINE_MODULE_REQUEST,
    .revision = 0,
};

__attribute__((
    used,
    section(
        ".requests"))) volatile struct limine_terminal_request early_term = {

    .id = LIMINE_TERMINAL_REQUEST, .revision = 0

};

__attribute__((
    used,
    section(
        ".requests"))) volatile struct limine_framebuffer_request fbr_req = {

    .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0

};

__attribute__((
    used,
    section(".requests"))) volatile struct limine_smp_request smp_request = {

    .id = LIMINE_SMP_REQUEST, .revision = 0

};

__attribute__((
    used,
    section(
        ".requests"))) volatile struct limine_stack_size_request stack_req = {

    .id = LIMINE_STACK_SIZE_REQUEST, .revision = 0, .stack_size = 4096

};

__attribute__((used,
               section(".requests"))) volatile struct limine_boot_time_request
    boot_time_req = {

        .id = LIMINE_BOOT_TIME_REQUEST, .revision = 0

};