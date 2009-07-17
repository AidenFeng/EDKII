## @file
# This file is used to define common static strings used by INF/DEC/DSC files
#
# Copyright (c) 2007 ~ 2008, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

##
# Common Definitions
#
TAB_SPLIT = '.'
TAB_COMMENT_R8_START = '/*'
TAB_COMMENT_R8_END = '*/'
TAB_COMMENT_R8_SPLIT = '//'
TAB_COMMENT_SPLIT = '#'
TAB_EQUAL_SPLIT = '='
TAB_VALUE_SPLIT = '|'
TAB_COMMA_SPLIT = ','
TAB_SPACE_SPLIT = ' '
TAB_SECTION_START = '['
TAB_SECTION_END = ']'
TAB_OPTION_START = '<'
TAB_OPTION_END  = '>'
TAB_SLASH = '\\'
TAB_BACK_SLASH = '/'

TAB_EDK_SOURCE = '$(EDK_SOURCE)'
TAB_EFI_SOURCE = '$(EFI_SOURCE)'
TAB_WORKSPACE = '$(WORKSPACE)'

TAB_ARCH_NULL = ''
TAB_ARCH_COMMON = 'COMMON'
TAB_ARCH_IA32 = 'IA32'
TAB_ARCH_X64 = 'X64'
TAB_ARCH_IPF = 'IPF'
TAB_ARCH_ARM = 'ARM'
TAB_ARCH_EBC = 'EBC'

ARCH_LIST = [TAB_ARCH_IA32, TAB_ARCH_X64, TAB_ARCH_IPF, TAB_ARCH_ARM, TAB_ARCH_EBC]
ARCH_LIST_FULL = [TAB_ARCH_COMMON] + ARCH_LIST

SUP_MODULE_BASE = 'BASE'
SUP_MODULE_SEC = 'SEC'
SUP_MODULE_PEI_CORE = 'PEI_CORE'
SUP_MODULE_PEIM = 'PEIM'
SUP_MODULE_DXE_CORE = 'DXE_CORE'
SUP_MODULE_DXE_DRIVER = 'DXE_DRIVER'
SUP_MODULE_DXE_RUNTIME_DRIVER = 'DXE_RUNTIME_DRIVER'
SUP_MODULE_DXE_SAL_DRIVER = 'DXE_SAL_DRIVER'
SUP_MODULE_DXE_SMM_DRIVER = 'DXE_SMM_DRIVER'
SUP_MODULE_UEFI_DRIVER = 'UEFI_DRIVER'
SUP_MODULE_UEFI_APPLICATION = 'UEFI_APPLICATION'
SUP_MODULE_USER_DEFINED = 'USER_DEFINED'
SUP_MODULE_SMM_DRIVER = 'SMM_DRIVER'
SUP_MODULE_SMM_CORE = 'SMM_CORE'

SUP_MODULE_LIST = [SUP_MODULE_BASE, SUP_MODULE_SEC, SUP_MODULE_PEI_CORE, SUP_MODULE_PEIM, SUP_MODULE_DXE_CORE, SUP_MODULE_DXE_DRIVER, \
                   SUP_MODULE_DXE_RUNTIME_DRIVER, SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_DXE_SMM_DRIVER, SUP_MODULE_UEFI_DRIVER, \
                   SUP_MODULE_UEFI_APPLICATION, SUP_MODULE_USER_DEFINED, SUP_MODULE_SMM_DRIVER, SUP_MODULE_SMM_CORE]
SUP_MODULE_LIST_STRING = TAB_VALUE_SPLIT.join(l for l in SUP_MODULE_LIST)

EDK_COMPONENT_TYPE_LIBRARY = 'LIBRARY'
EDK_COMPONENT_TYPE_SECUARITY_CORE = 'SECUARITY_CORE'
EDK_COMPONENT_TYPE_PEI_CORE = 'PEI_CORE'
EDK_COMPONENT_TYPE_COMBINED_PEIM_DRIVER = 'COMBINED_PEIM_DRIVER'
EDK_COMPONENT_TYPE_PIC_PEIM = 'PIC_PEIM'
EDK_COMPONENT_TYPE_RELOCATABLE_PEIM = 'RELOCATABLE_PEIM'
EDK_COMPONENT_TYPE_BS_DRIVER = 'BS_DRIVER'
EDK_COMPONENT_TYPE_RT_DRIVER = 'RT_DRIVER'
EDK_COMPONENT_TYPE_SAL_RT_DRIVER = 'SAL_RT_DRIVER'
EDK_COMPONENT_TYPE_APPLICATION = 'APPLICATION'

BINARY_FILE_TYPE_FW = 'FW'
BINARY_FILE_TYPE_GUID = 'GUID'
BINARY_FILE_TYPE_PREEFORM = 'PREEFORM'
BINARY_FILE_TYPE_UEFI_APP = 'UEFI_APP'
BINARY_FILE_TYPE_UNI_UI = 'UNI_UI'
BINARY_FILE_TYPE_UNI_VER = 'UNI_VER'
BINARY_FILE_TYPE_LIB = 'LIB'
BINARY_FILE_TYPE_PE32 = 'PE32'
BINARY_FILE_TYPE_PIC = 'PIC'
BINARY_FILE_TYPE_PEI_DEPEX = 'PEI_DEPEX'
BINARY_FILE_TYPE_DXE_DEPEX = 'DXE_DEPEX'
BINARY_FILE_TYPE_TE = 'TE'
BINARY_FILE_TYPE_VER = 'VER'
BINARY_FILE_TYPE_UI = 'UI'
BINARY_FILE_TYPE_BIN = 'BIN'
BINARY_FILE_TYPE_FV = 'FV'

PLATFORM_COMPONENT_TYPE_LIBRARY = 'LIBRARY'
PLATFORM_COMPONENT_TYPE_LIBRARY_CLASS = 'LIBRARY_CLASS'
PLATFORM_COMPONENT_TYPE_MODULE = 'MODULE'

TAB_LIBRARIES = 'Libraries'

TAB_SOURCES = 'Sources'
TAB_SOURCES_COMMON = TAB_SOURCES + TAB_SPLIT + TAB_ARCH_COMMON
TAB_SOURCES_IA32 = TAB_SOURCES + TAB_SPLIT + TAB_ARCH_IA32
TAB_SOURCES_X64 = TAB_SOURCES + TAB_SPLIT + TAB_ARCH_X64
TAB_SOURCES_IPF = TAB_SOURCES + TAB_SPLIT + TAB_ARCH_IPF
TAB_SOURCES_ARM = TAB_SOURCES + TAB_SPLIT + TAB_ARCH_ARM
TAB_SOURCES_EBC = TAB_SOURCES + TAB_SPLIT + TAB_ARCH_EBC

TAB_BINARIES = 'Binaries'
TAB_BINARIES_COMMON = TAB_BINARIES + TAB_SPLIT + TAB_ARCH_COMMON
TAB_BINARIES_IA32 = TAB_BINARIES + TAB_SPLIT + TAB_ARCH_IA32
TAB_BINARIES_X64 = TAB_BINARIES + TAB_SPLIT + TAB_ARCH_X64
TAB_BINARIES_IPF = TAB_BINARIES + TAB_SPLIT + TAB_ARCH_IPF
TAB_BINARIES_ARM = TAB_BINARIES + TAB_SPLIT + TAB_ARCH_ARM
TAB_BINARIES_EBC = TAB_BINARIES + TAB_SPLIT + TAB_ARCH_EBC

TAB_INCLUDES = 'Includes'
TAB_INCLUDES_COMMON = TAB_INCLUDES + TAB_SPLIT + TAB_ARCH_COMMON
TAB_INCLUDES_IA32 = TAB_INCLUDES + TAB_SPLIT + TAB_ARCH_IA32
TAB_INCLUDES_X64 = TAB_INCLUDES + TAB_SPLIT + TAB_ARCH_X64
TAB_INCLUDES_IPF = TAB_INCLUDES + TAB_SPLIT + TAB_ARCH_IPF
TAB_INCLUDES_ARM = TAB_INCLUDES + TAB_SPLIT + TAB_ARCH_ARM
TAB_INCLUDES_EBC = TAB_INCLUDES + TAB_SPLIT + TAB_ARCH_EBC

TAB_GUIDS = 'Guids'
TAB_GUIDS_COMMON = TAB_GUIDS + TAB_SPLIT + TAB_ARCH_COMMON
TAB_GUIDS_IA32 = TAB_GUIDS + TAB_SPLIT + TAB_ARCH_IA32
TAB_GUIDS_X64 = TAB_GUIDS + TAB_SPLIT + TAB_ARCH_X64
TAB_GUIDS_IPF = TAB_GUIDS + TAB_SPLIT + TAB_ARCH_IPF
TAB_GUIDS_ARM = TAB_GUIDS + TAB_SPLIT + TAB_ARCH_ARM
TAB_GUIDS_EBC = TAB_GUIDS + TAB_SPLIT + TAB_ARCH_EBC

TAB_PROTOCOLS = 'Protocols'
TAB_PROTOCOLS_COMMON = TAB_PROTOCOLS + TAB_SPLIT + TAB_ARCH_COMMON
TAB_PROTOCOLS_IA32 = TAB_PROTOCOLS + TAB_SPLIT + TAB_ARCH_IA32
TAB_PROTOCOLS_X64 = TAB_PROTOCOLS + TAB_SPLIT + TAB_ARCH_X64
TAB_PROTOCOLS_IPF = TAB_PROTOCOLS + TAB_SPLIT + TAB_ARCH_IPF
TAB_PROTOCOLS_ARM = TAB_PROTOCOLS + TAB_SPLIT + TAB_ARCH_ARM
TAB_PROTOCOLS_EBC = TAB_PROTOCOLS + TAB_SPLIT + TAB_ARCH_EBC

TAB_PPIS = 'Ppis'
TAB_PPIS_COMMON = TAB_PPIS + TAB_SPLIT + TAB_ARCH_COMMON
TAB_PPIS_IA32 = TAB_PPIS + TAB_SPLIT + TAB_ARCH_IA32
TAB_PPIS_X64 = TAB_PPIS + TAB_SPLIT + TAB_ARCH_X64
TAB_PPIS_IPF = TAB_PPIS + TAB_SPLIT + TAB_ARCH_IPF
TAB_PPIS_ARM = TAB_PPIS + TAB_SPLIT + TAB_ARCH_ARM
TAB_PPIS_EBC = TAB_PPIS + TAB_SPLIT + TAB_ARCH_EBC

TAB_LIBRARY_CLASSES = 'LibraryClasses'
TAB_LIBRARY_CLASSES_COMMON = TAB_LIBRARY_CLASSES + TAB_SPLIT + TAB_ARCH_COMMON
TAB_LIBRARY_CLASSES_IA32 = TAB_LIBRARY_CLASSES + TAB_SPLIT + TAB_ARCH_IA32
TAB_LIBRARY_CLASSES_X64 = TAB_LIBRARY_CLASSES + TAB_SPLIT + TAB_ARCH_X64
TAB_LIBRARY_CLASSES_IPF = TAB_LIBRARY_CLASSES + TAB_SPLIT + TAB_ARCH_IPF
TAB_LIBRARY_CLASSES_ARM = TAB_LIBRARY_CLASSES + TAB_SPLIT + TAB_ARCH_ARM
TAB_LIBRARY_CLASSES_EBC = TAB_LIBRARY_CLASSES + TAB_SPLIT + TAB_ARCH_EBC

TAB_PACKAGES = 'Packages'
TAB_PACKAGES_COMMON = TAB_PACKAGES + TAB_SPLIT + TAB_ARCH_COMMON
TAB_PACKAGES_IA32 = TAB_PACKAGES + TAB_SPLIT + TAB_ARCH_IA32
TAB_PACKAGES_X64 = TAB_PACKAGES + TAB_SPLIT + TAB_ARCH_X64
TAB_PACKAGES_IPF = TAB_PACKAGES + TAB_SPLIT + TAB_ARCH_IPF
TAB_PACKAGES_ARM = TAB_PACKAGES + TAB_SPLIT + TAB_ARCH_ARM
TAB_PACKAGES_EBC = TAB_PACKAGES + TAB_SPLIT + TAB_ARCH_EBC

TAB_PCDS = 'Pcds'
TAB_PCDS_FIXED_AT_BUILD = 'FixedAtBuild'
TAB_PCDS_PATCHABLE_IN_MODULE = 'PatchableInModule'
TAB_PCDS_FEATURE_FLAG = 'FeatureFlag'
TAB_PCDS_DYNAMIC_EX = 'DynamicEx'
TAB_PCDS_DYNAMIC_EX_DEFAULT = 'DynamicExDefault'
TAB_PCDS_DYNAMIC_EX_VPD = 'DynamicExVpd'
TAB_PCDS_DYNAMIC_EX_HII = 'DynamicExHii'
TAB_PCDS_DYNAMIC = 'Dynamic'
TAB_PCDS_DYNAMIC_DEFAULT = 'DynamicDefault'
TAB_PCDS_DYNAMIC_VPD = 'DynamicVpd'
TAB_PCDS_DYNAMIC_HII = 'DynamicHii'

PCD_DYNAMIC_TYPE_LIST = [TAB_PCDS_DYNAMIC, TAB_PCDS_DYNAMIC_DEFAULT, TAB_PCDS_DYNAMIC_VPD, TAB_PCDS_DYNAMIC_HII]
PCD_DYNAMIC_EX_TYPE_LIST = [TAB_PCDS_DYNAMIC_EX, TAB_PCDS_DYNAMIC_EX_DEFAULT, TAB_PCDS_DYNAMIC_EX_VPD, TAB_PCDS_DYNAMIC_EX_HII]

## Dynamic-ex PCD types
gDynamicExPcd = [TAB_PCDS_DYNAMIC_EX, TAB_PCDS_DYNAMIC_EX_DEFAULT, TAB_PCDS_DYNAMIC_EX_VPD, TAB_PCDS_DYNAMIC_EX_HII]

TAB_PCDS_FIXED_AT_BUILD_NULL = TAB_PCDS + TAB_PCDS_FIXED_AT_BUILD
TAB_PCDS_FIXED_AT_BUILD_COMMON = TAB_PCDS + TAB_PCDS_FIXED_AT_BUILD + TAB_SPLIT + TAB_ARCH_COMMON
TAB_PCDS_FIXED_AT_BUILD_IA32 = TAB_PCDS + TAB_PCDS_FIXED_AT_BUILD + TAB_SPLIT + TAB_ARCH_IA32
TAB_PCDS_FIXED_AT_BUILD_X64 = TAB_PCDS + TAB_PCDS_FIXED_AT_BUILD + TAB_SPLIT + TAB_ARCH_X64
TAB_PCDS_FIXED_AT_BUILD_IPF = TAB_PCDS + TAB_PCDS_FIXED_AT_BUILD + TAB_SPLIT + TAB_ARCH_IPF
TAB_PCDS_FIXED_AT_BUILD_ARM = TAB_PCDS + TAB_PCDS_FIXED_AT_BUILD + TAB_SPLIT + TAB_ARCH_ARM
TAB_PCDS_FIXED_AT_BUILD_EBC = TAB_PCDS + TAB_PCDS_FIXED_AT_BUILD + TAB_SPLIT + TAB_ARCH_EBC

TAB_PCDS_PATCHABLE_IN_MODULE_NULL = TAB_PCDS + TAB_PCDS_PATCHABLE_IN_MODULE
TAB_PCDS_PATCHABLE_IN_MODULE_COMMON = TAB_PCDS + TAB_PCDS_PATCHABLE_IN_MODULE + TAB_SPLIT + TAB_ARCH_COMMON
TAB_PCDS_PATCHABLE_IN_MODULE_IA32 = TAB_PCDS + TAB_PCDS_PATCHABLE_IN_MODULE + TAB_SPLIT + TAB_ARCH_IA32
TAB_PCDS_PATCHABLE_IN_MODULE_X64 = TAB_PCDS + TAB_PCDS_PATCHABLE_IN_MODULE + TAB_SPLIT + TAB_ARCH_X64
TAB_PCDS_PATCHABLE_IN_MODULE_IPF = TAB_PCDS + TAB_PCDS_PATCHABLE_IN_MODULE + TAB_SPLIT + TAB_ARCH_IPF
TAB_PCDS_PATCHABLE_IN_MODULE_ARM = TAB_PCDS + TAB_PCDS_PATCHABLE_IN_MODULE + TAB_SPLIT + TAB_ARCH_ARM
TAB_PCDS_PATCHABLE_IN_MODULE_EBC = TAB_PCDS + TAB_PCDS_PATCHABLE_IN_MODULE + TAB_SPLIT + TAB_ARCH_EBC

TAB_PCDS_FEATURE_FLAG_NULL = TAB_PCDS + TAB_PCDS_FEATURE_FLAG
TAB_PCDS_FEATURE_FLAG_COMMON = TAB_PCDS + TAB_PCDS_FEATURE_FLAG + TAB_SPLIT + TAB_ARCH_COMMON
TAB_PCDS_FEATURE_FLAG_IA32 = TAB_PCDS + TAB_PCDS_FEATURE_FLAG + TAB_SPLIT + TAB_ARCH_IA32
TAB_PCDS_FEATURE_FLAG_X64 = TAB_PCDS + TAB_PCDS_FEATURE_FLAG + TAB_SPLIT + TAB_ARCH_X64
TAB_PCDS_FEATURE_FLAG_IPF = TAB_PCDS + TAB_PCDS_FEATURE_FLAG + TAB_SPLIT + TAB_ARCH_IPF
TAB_PCDS_FEATURE_FLAG_ARM = TAB_PCDS + TAB_PCDS_FEATURE_FLAG + TAB_SPLIT + TAB_ARCH_ARM
TAB_PCDS_FEATURE_FLAG_EBC = TAB_PCDS + TAB_PCDS_FEATURE_FLAG + TAB_SPLIT + TAB_ARCH_EBC

TAB_PCDS_DYNAMIC_EX_NULL = TAB_PCDS + TAB_PCDS_DYNAMIC_EX
TAB_PCDS_DYNAMIC_EX_DEFAULT_NULL = TAB_PCDS + TAB_PCDS_DYNAMIC_EX_DEFAULT
TAB_PCDS_DYNAMIC_EX_HII_NULL = TAB_PCDS + TAB_PCDS_DYNAMIC_EX_HII
TAB_PCDS_DYNAMIC_EX_VPD_NULL = TAB_PCDS + TAB_PCDS_DYNAMIC_EX_VPD
TAB_PCDS_DYNAMIC_EX_COMMON = TAB_PCDS + TAB_PCDS_DYNAMIC_EX + TAB_SPLIT + TAB_ARCH_COMMON
TAB_PCDS_DYNAMIC_EX_IA32 = TAB_PCDS + TAB_PCDS_DYNAMIC_EX + TAB_SPLIT + TAB_ARCH_IA32
TAB_PCDS_DYNAMIC_EX_X64 = TAB_PCDS + TAB_PCDS_DYNAMIC_EX + TAB_SPLIT + TAB_ARCH_X64
TAB_PCDS_DYNAMIC_EX_IPF = TAB_PCDS + TAB_PCDS_DYNAMIC_EX + TAB_SPLIT + TAB_ARCH_IPF
TAB_PCDS_DYNAMIC_EX_ARM = TAB_PCDS + TAB_PCDS_DYNAMIC_EX + TAB_SPLIT + TAB_ARCH_ARM
TAB_PCDS_DYNAMIC_EX_EBC = TAB_PCDS + TAB_PCDS_DYNAMIC_EX + TAB_SPLIT + TAB_ARCH_EBC

TAB_PCDS_DYNAMIC_NULL = TAB_PCDS + TAB_PCDS_DYNAMIC
TAB_PCDS_DYNAMIC_DEFAULT_NULL = TAB_PCDS + TAB_PCDS_DYNAMIC_DEFAULT
TAB_PCDS_DYNAMIC_HII_NULL = TAB_PCDS + TAB_PCDS_DYNAMIC_HII
TAB_PCDS_DYNAMIC_VPD_NULL = TAB_PCDS + TAB_PCDS_DYNAMIC_VPD
TAB_PCDS_DYNAMIC_COMMON = TAB_PCDS + TAB_PCDS_DYNAMIC + TAB_SPLIT + TAB_ARCH_COMMON
TAB_PCDS_DYNAMIC_IA32 = TAB_PCDS + TAB_PCDS_DYNAMIC + TAB_SPLIT + TAB_ARCH_IA32
TAB_PCDS_DYNAMIC_X64 = TAB_PCDS + TAB_PCDS_DYNAMIC + TAB_SPLIT + TAB_ARCH_X64
TAB_PCDS_DYNAMIC_IPF = TAB_PCDS + TAB_PCDS_DYNAMIC + TAB_SPLIT + TAB_ARCH_IPF
TAB_PCDS_DYNAMIC_ARM = TAB_PCDS + TAB_PCDS_DYNAMIC + TAB_SPLIT + TAB_ARCH_ARM
TAB_PCDS_DYNAMIC_EBC = TAB_PCDS + TAB_PCDS_DYNAMIC + TAB_SPLIT + TAB_ARCH_EBC

TAB_PCD_DYNAMIC_TYPE_LIST = [TAB_PCDS_DYNAMIC_DEFAULT_NULL, TAB_PCDS_DYNAMIC_VPD_NULL, TAB_PCDS_DYNAMIC_HII_NULL]
TAB_PCD_DYNAMIC_EX_TYPE_LIST = [TAB_PCDS_DYNAMIC_EX_DEFAULT_NULL, TAB_PCDS_DYNAMIC_EX_VPD_NULL, TAB_PCDS_DYNAMIC_EX_HII_NULL]

TAB_DEPEX = 'Depex'
TAB_DEPEX_COMMON = TAB_DEPEX + TAB_SPLIT + TAB_ARCH_COMMON
TAB_DEPEX_IA32 = TAB_DEPEX + TAB_SPLIT + TAB_ARCH_IA32
TAB_DEPEX_X64 = TAB_DEPEX + TAB_SPLIT + TAB_ARCH_X64
TAB_DEPEX_IPF = TAB_DEPEX + TAB_SPLIT + TAB_ARCH_IPF
TAB_DEPEX_ARM = TAB_DEPEX + TAB_SPLIT + TAB_ARCH_ARM
TAB_DEPEX_EBC = TAB_DEPEX + TAB_SPLIT + TAB_ARCH_EBC

TAB_SKUIDS = 'SkuIds'

TAB_LIBRARIES = 'Libraries'
TAB_LIBRARIES_COMMON = TAB_LIBRARIES + TAB_SPLIT + TAB_ARCH_COMMON
TAB_LIBRARIES_IA32 = TAB_LIBRARIES + TAB_SPLIT + TAB_ARCH_IA32
TAB_LIBRARIES_X64 = TAB_LIBRARIES + TAB_SPLIT + TAB_ARCH_X64
TAB_LIBRARIES_IPF = TAB_LIBRARIES + TAB_SPLIT + TAB_ARCH_IPF
TAB_LIBRARIES_ARM = TAB_LIBRARIES + TAB_SPLIT + TAB_ARCH_ARM
TAB_LIBRARIES_EBC = TAB_LIBRARIES + TAB_SPLIT + TAB_ARCH_EBC

TAB_COMPONENTS = 'Components'
TAB_COMPONENTS_COMMON = TAB_COMPONENTS + TAB_SPLIT + TAB_ARCH_COMMON
TAB_COMPONENTS_IA32 = TAB_COMPONENTS + TAB_SPLIT + TAB_ARCH_IA32
TAB_COMPONENTS_X64 = TAB_COMPONENTS + TAB_SPLIT + TAB_ARCH_X64
TAB_COMPONENTS_IPF = TAB_COMPONENTS + TAB_SPLIT + TAB_ARCH_IPF
TAB_COMPONENTS_ARM = TAB_COMPONENTS + TAB_SPLIT + TAB_ARCH_ARM
TAB_COMPONENTS_EBC = TAB_COMPONENTS + TAB_SPLIT + TAB_ARCH_EBC

TAB_COMPONENTS_SOURCE_OVERRIDE_PATH = 'SOURCE_OVERRIDE_PATH'

TAB_BUILD_OPTIONS = 'BuildOptions'

TAB_DEFINE = 'DEFINE'
TAB_NMAKE = 'Nmake'
TAB_USER_EXTENSIONS = 'UserExtensions'
TAB_INCLUDE = '!include'

#
# Common Define
#
TAB_COMMON_DEFINES = 'Defines'

#
# Inf Definitions
#
TAB_INF_DEFINES = TAB_COMMON_DEFINES
TAB_INF_DEFINES_INF_VERSION = 'INF_VERSION'
TAB_INF_DEFINES_BASE_NAME = 'BASE_NAME'
TAB_INF_DEFINES_FILE_GUID = 'FILE_GUID'
TAB_INF_DEFINES_MODULE_TYPE = 'MODULE_TYPE'
TAB_INF_DEFINES_EFI_SPECIFICATION_VERSION = 'EFI_SPECIFICATION_VERSION'
TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION = 'UEFI_SPECIFICATION_VERSION'
TAB_INF_DEFINES_PI_SPECIFICATION_VERSION = 'PI_SPECIFICATION_VERSION'
TAB_INF_DEFINES_EDK_RELEASE_VERSION = 'EDK_RELEASE_VERSION'
TAB_INF_DEFINES_BINARY_MODULE = 'BINARY_MODULE'
TAB_INF_DEFINES_LIBRARY_CLASS = 'LIBRARY_CLASS'
TAB_INF_DEFINES_COMPONENT_TYPE = 'COMPONENT_TYPE'
TAB_INF_DEFINES_MAKEFILE_NAME = 'MAKEFILE_NAME'
TAB_INF_DEFINES_BUILD_NUMBER = 'BUILD_NUMBER'
TAB_INF_DEFINES_BUILD_TYPE = 'BUILD_TYPE'
TAB_INF_DEFINES_FFS_EXT = 'FFS_EXT'
TAB_INF_DEFINES_FV_EXT = 'FV_EXT'
TAB_INF_DEFINES_SOURCE_FV = 'SOURCE_FV'
TAB_INF_DEFINES_VERSION_NUMBER = 'VERSION_NUMBER'
TAB_INF_DEFINES_VERSION = 'VERSION'          # for R8 inf, the same as VERSION_NUMBER
TAB_INF_DEFINES_VERSION_STRING = 'VERSION_STRING'
TAB_INF_DEFINES_PCD_IS_DRIVER = 'PCD_IS_DRIVER'
TAB_INF_DEFINES_TIANO_R8_FLASHMAP_H = 'TIANO_R8_FLASHMAP_H'
TAB_INF_DEFINES_ENTRY_POINT = 'ENTRY_POINT'
TAB_INF_DEFINES_UNLOAD_IMAGE = 'UNLOAD_IMAGE'
TAB_INF_DEFINES_CONSTRUCTOR = 'CONSTRUCTOR'
TAB_INF_DEFINES_DESTRUCTOR = 'DESTRUCTOR'
TAB_INF_DEFINES_DEFINE = 'DEFINE'
TAB_INF_DEFINES_SPEC = 'SPEC'
TAB_INF_DEFINES_CUSTOM_MAKEFILE = 'CUSTOM_MAKEFILE'
TAB_INF_DEFINES_MACRO = '__MACROS__'
TAB_INF_DEFINES_SHADOW = 'SHADOW'
TAB_INF_FIXED_PCD = 'FixedPcd'
TAB_INF_FEATURE_PCD = 'FeaturePcd'
TAB_INF_PATCH_PCD = 'PatchPcd'
TAB_INF_PCD = 'Pcd'
TAB_INF_PCD_EX = 'PcdEx'

#
# Dec Definitions
#
TAB_DEC_DEFINES = TAB_COMMON_DEFINES
TAB_DEC_DEFINES_DEC_SPECIFICATION = 'DEC_SPECIFICATION'
TAB_DEC_DEFINES_PACKAGE_NAME = 'PACKAGE_NAME'
TAB_DEC_DEFINES_PACKAGE_GUID = 'PACKAGE_GUID'
TAB_DEC_DEFINES_PACKAGE_VERSION = 'PACKAGE_VERSION'

#
# Dsc Definitions
#
TAB_DSC_DEFINES = TAB_COMMON_DEFINES
TAB_DSC_DEFINES_PLATFORM_NAME = 'PLATFORM_NAME'
TAB_DSC_DEFINES_PLATFORM_GUID = 'PLATFORM_GUID'
TAB_DSC_DEFINES_PLATFORM_VERSION = 'PLATFORM_VERSION'
TAB_DSC_DEFINES_DSC_SPECIFICATION = 'DSC_SPECIFICATION'
TAB_DSC_DEFINES_OUTPUT_DIRECTORY = 'OUTPUT_DIRECTORY'
TAB_DSC_DEFINES_SUPPORTED_ARCHITECTURES = 'SUPPORTED_ARCHITECTURES'
TAB_DSC_DEFINES_BUILD_TARGETS = 'BUILD_TARGETS'
TAB_DSC_DEFINES_SKUID_IDENTIFIER = 'SKUID_IDENTIFIER'
TAB_DSC_DEFINES_FLASH_DEFINITION = 'FLASH_DEFINITION'
TAB_DSC_DEFINES_BUILD_NUMBER = 'BUILD_NUMBER'
TAB_DSC_DEFINES_MAKEFILE_NAME = 'MAKEFILE_NAME'
TAB_DSC_DEFINES_BS_BASE_ADDRESS = 'BsBaseAddress'
TAB_DSC_DEFINES_RT_BASE_ADDRESS = 'RtBaseAddress'
TAB_DSC_DEFINES_DEFINE = 'DEFINE'

#
# TargetTxt Definitions
#
TAB_TAT_DEFINES_ACTIVE_PLATFORM = 'ACTIVE_PLATFORM'
TAB_TAT_DEFINES_ACTIVE_MODULE = 'ACTIVE_MODULE'
TAB_TAT_DEFINES_TOOL_CHAIN_CONF = 'TOOL_CHAIN_CONF'
TAB_TAT_DEFINES_MULTIPLE_THREAD = 'MULTIPLE_THREAD'
TAB_TAT_DEFINES_MAX_CONCURRENT_THREAD_NUMBER = 'MAX_CONCURRENT_THREAD_NUMBER'
TAB_TAT_DEFINES_TARGET = 'TARGET'
TAB_TAT_DEFINES_TOOL_CHAIN_TAG = 'TOOL_CHAIN_TAG'
TAB_TAT_DEFINES_TARGET_ARCH = 'TARGET_ARCH'
TAB_TAT_DEFINES_BUILD_RULE_CONF = "BUILD_RULE_CONF"

#
# ToolDef Definitions
#
TAB_TOD_DEFINES_TARGET = 'TARGET'
TAB_TOD_DEFINES_TOOL_CHAIN_TAG = 'TOOL_CHAIN_TAG'
TAB_TOD_DEFINES_TARGET_ARCH = 'TARGET_ARCH'
TAB_TOD_DEFINES_COMMAND_TYPE = 'COMMAND_TYPE'
TAB_TOD_DEFINES_FAMILY = 'FAMILY'
TAB_TOD_DEFINES_BUILDRULEFAMILY = 'BUILDRULEFAMILY'

#
# Conditional Statements
#
TAB_IF = '!if'
TAB_END_IF = '!endif'
TAB_ELSE_IF = '!elseif'
TAB_ELSE = '!else'
TAB_IF_DEF = '!ifdef'
TAB_IF_N_DEF = '!ifndef'
TAB_IF_EXIST = '!if exist'

#
# Unknown section
#
TAB_UNKNOWN = 'UNKNOWN'

#
# Build database path
#
DATABASE_PATH = ":memory:" #"BuildDatabase.db"

# used by ECC
MODIFIER_LIST = ['IN', 'OUT', 'OPTIONAL', 'UNALIGNED', 'EFI_RUNTIMESERVICE', 'EFI_BOOTSERVICE', 'EFIAPI']

# Dependency Expression
DEPEX_SUPPORTED_OPCODE = ["BEFORE", "AFTER", "PUSH", "AND", "OR", "NOT", "END", "SOR", "TRUE", "FALSE", '(', ')']

TAB_STATIC_LIBRARY = "STATIC-LIBRARY-FILE"
TAB_DYNAMIC_LIBRARY = "DYNAMIC-LIBRARY-FILE"
TAB_FRAMEWORK_IMAGE = "EFI-IMAGE-FILE"
TAB_C_CODE_FILE = "C-CODE-FILE"
TAB_C_HEADER_FILE = "C-HEADER-FILE"
TAB_UNICODE_FILE = "UNICODE-TEXT-FILE"
TAB_DEPENDENCY_EXPRESSION_FILE = "DEPENDENCY-EXPRESSION-FILE"
TAB_UNKNOWN_FILE = "UNKNOWN-TYPE-FILE"
TAB_DEFAULT_BINARY_FILE = "_BINARY_FILE_"

