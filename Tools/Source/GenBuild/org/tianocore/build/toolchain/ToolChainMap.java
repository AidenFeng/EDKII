/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  ToolChainMap.java

Abstract:

--*/

package org.tianocore.build.toolchain;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.tianocore.build.exception.EdkException;

public class ToolChainMap {

    private int matchLevel = ToolChainKey.keyLength - 2;

    private Map<ToolChainKey, String> map = null;

    public ToolChainMap() {
        //this.map = new TreeMap<ToolChainKey, String>();
        this.map = new HashMap<ToolChainKey, String>();
    }

    public String put(String key, String delimiter, String value)throws EdkException {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key, delimiter);
        } catch (Exception e) {
            throw new EdkException(e.getMessage());
        }
        return (String)map.put(toolChainKey, value);
    }

    public String put(String key, String value) throws EdkException {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key);
        } catch (Exception e) {
            throw new EdkException(e.getMessage());
        }
        return (String)map.put(toolChainKey, value);
    }

    public String put(String[] key, String value) throws EdkException {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key);
        } catch (Exception e) {
            throw new EdkException(e.getMessage());
        }
        return (String)map.put(toolChainKey, value);
    }

    public String put(ToolChainKey key, String value) {
        return (String)map.put(key, value);
    }

    public String get(String key) throws EdkException {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key);
        } catch (Exception e) {
            throw new EdkException(e.getMessage());
        }
        return get(toolChainKey);
    }

    public String get(String key, String delimiter) throws EdkException {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key, delimiter);
        } catch (Exception e) {
            throw new EdkException(e.getMessage());
        }
        return get(toolChainKey);
    }

    public String get(String[] key)  throws EdkException {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key);
        } catch (Exception e) {
            throw new EdkException(e.getMessage());
        }
        return get(toolChainKey);
    }

    public String get(ToolChainKey key)  throws EdkException {
        String result = map.get(key);
        if (result != null || map.containsKey(key)) {
            return result;
        }

        String[] keySet = key.getKeySet();
        ToolChainKey tmpKey;
        try {
            tmpKey = new ToolChainKey(keySet);
        } catch (Exception e) {
            throw new EdkException(e.getMessage());
        }

        int level = matchLevel;
        while (level >= 0) {
            int tmpLevel = level;
            while (tmpLevel >= level) {
                String[] tmpKeySet = tmpKey.getKeySet();
                try {
                    if (!tmpKeySet[tmpLevel].equals("*")) {
                        tmpKey.setKey("*", tmpLevel);
                        tmpLevel = matchLevel;
                    } else {
                        tmpKey.setKey(keySet[tmpLevel], tmpLevel);
                        --tmpLevel;
                        continue;
                    }
                } catch (Exception e) {
                    throw new EdkException(e.getMessage());
                }

                result = map.get(tmpKey);
                if (result != null) {
                    map.put(key, result);
                    return result;
                }
            }
            --level;
        }

        map.put(key, result);
        return result;
    }

    public int size() {
        return map.size();
    }

    public Set<ToolChainKey> keySet() {
        return (Set<ToolChainKey>)map.keySet();
    }
    
//    public String toString() {
//        return map.toString();
//    }
}

