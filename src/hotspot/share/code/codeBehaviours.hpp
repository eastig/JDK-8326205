/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates. All rights reserved.
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
 *
 */

#ifndef SHARE_CODE_CODEBEHAVIOURS_HPP
#define SHARE_CODE_CODEBEHAVIOURS_HPP

#include "memory/allocation.hpp"

class nmethod;

class CompiledICProtectionBehaviour {
  static CompiledICProtectionBehaviour* _current;

public:
  virtual bool lock(nmethod* nm) = 0;
  virtual void unlock(nmethod* nm) = 0;
  virtual bool is_safe(nmethod* nm) = 0;

  static CompiledICProtectionBehaviour* current() { return _current; }
  static void set_current(CompiledICProtectionBehaviour* current) { _current = current; }
};

class DefaultICProtectionBehaviour: public CompiledICProtectionBehaviour, public CHeapObj<mtInternal> {
  virtual bool lock(nmethod* nm);
  virtual void unlock(nmethod* nm);
  virtual bool is_safe(nmethod* nm);
};

#endif // SHARE_CODE_CODEBEHAVIOURS_HPP
