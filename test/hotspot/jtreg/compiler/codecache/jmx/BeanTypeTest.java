/*
 * Copyright (c) 2014, 2022, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

/**
 * @test BeanTypeTest
 * @summary verify types of code cache memory pool bean
 * @modules java.base/jdk.internal.misc
 *          java.management
 * @library /test/lib
 *
 * @build jdk.test.whitebox.WhiteBox
 * @run driver jdk.test.lib.helpers.ClassFileInstaller jdk.test.whitebox.WhiteBox
 * @run main/othervm -Xbootclasspath/a:. -XX:+UnlockDiagnosticVMOptions
 *     -XX:+WhiteBoxAPI
 *     -XX:+SegmentedCodeCache
 *     compiler.codecache.jmx.BeanTypeTest
 * @run main/othervm -Xbootclasspath/a:. -XX:+UnlockDiagnosticVMOptions
 *     -XX:+WhiteBoxAPI
 *     -XX:-SegmentedCodeCache
 *     compiler.codecache.jmx.BeanTypeTest
 * @run main/othervm -Xbootclasspath/a:. -XX:+UnlockDiagnosticVMOptions
 *     -XX:+WhiteBoxAPI
 *     -XX:HotCodeHeapSize=8M -XX:+TieredCompilation -XX:TieredStopAtLevel=4
 *     compiler.codecache.jmx.BeanTypeTest
 * @run main/othervm -Xbootclasspath/a:. -XX:+UnlockDiagnosticVMOptions
 *     -XX:+WhiteBoxAPI
 *     -XX:HotCodeHeapSize=8M -XX:-TieredCompilation -XX:TieredStopAtLevel=4
 *     compiler.codecache.jmx.BeanTypeTest
 */

package compiler.codecache.jmx;

import jdk.test.lib.Asserts;
import jdk.test.whitebox.code.BlobType;

import java.lang.management.MemoryType;

public class BeanTypeTest {

    public static void main(String args[]) {
        for (BlobType bt : BlobType.getAvailable()) {
            Asserts.assertEQ(MemoryType.NON_HEAP, bt.getMemoryPool().getType());
        }
    }
}
