/*
 * Copyright (c) 2000, 2025, Oracle and/or its affiliates. All rights reserved.
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

package sun.jvm.hotspot.code;

import java.io.*;
import java.util.*;
import sun.jvm.hotspot.debugger.*;
import sun.jvm.hotspot.memory.*;
import sun.jvm.hotspot.oops.*;
import sun.jvm.hotspot.runtime.*;
import sun.jvm.hotspot.types.*;
import sun.jvm.hotspot.utilities.*;
import sun.jvm.hotspot.utilities.Observable;
import sun.jvm.hotspot.utilities.Observer;

public class NMethod extends CodeBlob {
  private static long          pcDescSize;
  private static long          immutableDataReferencesCounterSize;
  private static AddressField  methodField;
  /** != InvocationEntryBci if this nmethod is an on-stack replacement method */
  private static CIntegerField entryBCIField;
  /** To support simple linked-list chaining of nmethods */
  private static AddressField  osrLinkField;
  private static AddressField  immutableDataField;
  private static CIntegerField immutableDataSizeField;

  /** Offsets for different nmethod parts */
  private static CIntegerField exceptionOffsetField;
  private static CIntegerField deoptHandlerOffsetField;
  private static CIntegerField deoptMhHandlerOffsetField;
  private static CIntegerField origPCOffsetField;
  private static CIntegerField stubOffsetField;
  private static CIntField     handlerTableOffsetField;
  private static CIntField     nulChkTableOffsetField;
  private static CIntegerField scopesPCsOffsetField;
  private static CIntegerField scopesDataOffsetField;

  /** Offsets for entry points */
  /** Entry point with class check */
  private static CIntField  entryOffsetField;
  /** Entry point without class check */
  private static CIntField  verifiedEntryOffsetField;
  /** Entry point for on stack replacement */
  private static AddressField  osrEntryPointField;

  // FIXME: add access to flags (how?)

  private static CIntField compLevelField;

  static {
    VM.registerVMInitializedObserver(new Observer() {
        public void update(Observable o, Object data) {
          initialize(VM.getVM().getTypeDataBase());
        }
      });
  }

  private static void initialize(TypeDataBase db) {
    Type type = db.lookupType("nmethod");

    methodField                        = type.getAddressField("_method");
    entryBCIField                      = type.getCIntegerField("_entry_bci");
    osrLinkField                       = type.getAddressField("_osr_link");
    immutableDataField                 = type.getAddressField("_immutable_data");
    immutableDataSizeField             = type.getCIntegerField("_immutable_data_size");
    exceptionOffsetField               = type.getCIntegerField("_exception_offset");
    deoptHandlerOffsetField            = type.getCIntegerField("_deopt_handler_offset");
    deoptMhHandlerOffsetField          = type.getCIntegerField("_deopt_mh_handler_offset");
    origPCOffsetField                  = type.getCIntegerField("_orig_pc_offset");
    stubOffsetField                    = type.getCIntegerField("_stub_offset");
    scopesPCsOffsetField               = type.getCIntegerField("_scopes_pcs_offset");
    scopesDataOffsetField              = type.getCIntegerField("_scopes_data_offset");
    handlerTableOffsetField            = new CIntField(type.getCIntegerField("_handler_table_offset"), 0);
    nulChkTableOffsetField             = new CIntField(type.getCIntegerField("_nul_chk_table_offset"), 0);
    entryOffsetField                   = new CIntField(type.getCIntegerField("_entry_offset"), 0);
    verifiedEntryOffsetField           = new CIntField(type.getCIntegerField("_verified_entry_offset"), 0);
    osrEntryPointField                 = type.getAddressField("_osr_entry_point");
    compLevelField                     = new CIntField(type.getCIntegerField("_comp_level"), 0);
    pcDescSize                         = db.lookupType("PcDesc").getSize();
    immutableDataReferencesCounterSize = VM.getVM().getIntSize();
  }

  public NMethod(Address addr) {
    super(addr);
  }

  // Accessors
  public Address getAddress() {
    return addr;
  }

  public Method getMethod() {
    return (Method)Metadata.instantiateWrapperFor(methodField.getValue(addr));
  }

  // Type info
  public boolean isJavaMethod()   { return !getMethod().isNative(); }
  public boolean isNativeMethod() { return getMethod().isNative();  }
  public boolean isOSRMethod()    { return getEntryBCI() != VM.getVM().getInvocationEntryBCI(); }

  /** Boundaries for different parts */
  public Address constantsBegin()       { return contentBegin();                                     }
  public Address constantsEnd()         { return codeBegin();                                        }
  public Address instsBegin()           { return codeBegin();                                        }
  public Address instsEnd()             { return headerBegin().addOffsetTo(getStubOffset());         }
  public Address exceptionBegin()       { return headerBegin().addOffsetTo(getExceptionOffset());    }
  public Address deoptHandlerBegin()    { return headerBegin().addOffsetTo(getDeoptHandlerOffset());   }
  public Address deoptMhHandlerBegin()  { return headerBegin().addOffsetTo(getDeoptMhHandlerOffset()); }
  public Address stubBegin()            { return headerBegin().addOffsetTo(getStubOffset());         }
  public Address stubEnd()              { return dataBegin();                                        }
  public Address oopsBegin()            { return dataBegin();                                        }
  public Address oopsEnd()              { return dataEnd();                                          }

  public Address immutableDataBegin()   { return immutableDataField.getValue(addr);                         }
  public Address immutableDataEnd()     { return immutableDataBegin().addOffsetTo(getImmutableDataSize());  }
  public Address dependenciesBegin()    { return immutableDataBegin();                                      }
  public Address dependenciesEnd()      { return immutableDataBegin().addOffsetTo(getHandlerTableOffset()); }
  public Address handlerTableBegin()    { return immutableDataBegin().addOffsetTo(getHandlerTableOffset()); }
  public Address handlerTableEnd()      { return immutableDataBegin().addOffsetTo(getNulChkTableOffset());  }
  public Address nulChkTableBegin()     { return immutableDataBegin().addOffsetTo(getNulChkTableOffset());  }
  public Address nulChkTableEnd()       { return immutableDataBegin().addOffsetTo(getScopesDataOffset());   }
  public Address scopesDataBegin()      { return immutableDataBegin().addOffsetTo(getScopesDataOffset());   }
  public Address scopesDataEnd()        { return immutableDataBegin().addOffsetTo(getScopesPCsOffset());    }
  public Address scopesPCsBegin()       { return immutableDataBegin().addOffsetTo(getScopesPCsOffset());    }
  public Address scopesPCsEnd()         { return immutableDataEnd().addOffsetTo(-immutableDataReferencesCounterSize); }

  public Address metadataBegin()        { return mutableDataBegin().addOffsetTo(getRelocationSize());   }
  public Address metadataEnd()          { return mutableDataEnd();                                      }

  public int getImmutableDataSize()     { return (int) immutableDataSizeField.getValue(addr);        }
  public int constantsSize()            { return (int) constantsEnd()   .minus(constantsBegin());    }
  public int instsSize()                { return (int) instsEnd()       .minus(instsBegin());        }
  public int stubSize()                 { return (int) stubEnd()        .minus(stubBegin());         }
  public int oopsSize()                 { return (int) oopsEnd()        .minus(oopsBegin());         }
  public int metadataSize()             { return (int) metadataEnd()    .minus(metadataBegin());     }
  public int scopesDataSize()           { return (int) scopesDataEnd()  .minus(scopesDataBegin());   }
  public int scopesPCsSize()            { return (int) scopesPCsEnd()   .minus(scopesPCsBegin());    }
  public int dependenciesSize()         { return (int) dependenciesEnd().minus(dependenciesBegin()); }
  public int handlerTableSize()         { return (int) handlerTableEnd().minus(handlerTableBegin()); }
  public int nulChkTableSize()          { return (int) nulChkTableEnd() .minus(nulChkTableBegin());  }
  public int origPCOffset()             { return (int) origPCOffsetField.getValue(addr);             }

  public int totalSize() {
    return
      constantsSize()    +
      instsSize()        +
      stubSize();
  }
  public int immutableDataSize() {
    return
      scopesDataSize()   +
      scopesPCsSize()    +
      dependenciesSize() +
      handlerTableSize() +
      nulChkTableSize()  +
      (int) immutableDataReferencesCounterSize;
  }

  public boolean constantsContains   (Address addr) { return constantsBegin()   .lessThanOrEqual(addr) && constantsEnd()   .greaterThan(addr); }
  public boolean instsContains       (Address addr) { return instsBegin()       .lessThanOrEqual(addr) && instsEnd()       .greaterThan(addr); }
  public boolean stubContains        (Address addr) { return stubBegin()        .lessThanOrEqual(addr) && stubEnd()        .greaterThan(addr); }
  public boolean oopsContains        (Address addr) { return oopsBegin()        .lessThanOrEqual(addr) && oopsEnd()        .greaterThan(addr); }
  public boolean metadataContains    (Address addr) { return metadataBegin()    .lessThanOrEqual(addr) && metadataEnd()    .greaterThan(addr); }
  public boolean scopesDataContains  (Address addr) { return scopesDataBegin()  .lessThanOrEqual(addr) && scopesDataEnd()  .greaterThan(addr); }
  public boolean scopesPCsContains   (Address addr) { return scopesPCsBegin()   .lessThanOrEqual(addr) && scopesPCsEnd()   .greaterThan(addr); }
  public boolean handlerTableContains(Address addr) { return handlerTableBegin().lessThanOrEqual(addr) && handlerTableEnd().greaterThan(addr); }
  public boolean nulChkTableContains (Address addr) { return nulChkTableBegin() .lessThanOrEqual(addr) && nulChkTableEnd() .greaterThan(addr); }

  public int getOopsLength() { return (int) (oopsSize() / VM.getVM().getOopSize()); }
  public int getMetadataLength() { return (int) (metadataSize() / VM.getVM().getOopSize()); }

  /** Entry points */
  public Address getEntryPoint()         { return codeBegin().addOffsetTo(getEntryPointOffset());         }
  public Address getVerifiedEntryPoint() { return codeBegin().addOffsetTo(getVerifiedEntryPointOffset()); }

  /** Support for oops in scopes and relocs. Note: index 0 is reserved for null. */
  public OopHandle getOopAt(int index) {
    if (index == 0) return null;
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(index > 0 && index <= getOopsLength(), "must be a valid non-zero index: " + index);
    }
    return oopsBegin().getOopHandleAt((index - 1) * VM.getVM().getOopSize());
  }

  /** Support for metadata in scopes and relocs. Note: index 0 is reserved for null. */
  public Address getMetadataAt(int index) {
    if (index == 0) return null;
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(index > 0 && index <= getMetadataLength(), "must be a valid non-zero index: " + index);
    }
    return metadataBegin().getAddressAt((index - 1) * VM.getVM().getOopSize());
  }

  public Method getMethodAt(int index) {
    return (Method)Metadata.instantiateWrapperFor(getMetadataAt(index));
  }

  // FIXME: add interpreter_entry_point()
  // FIXME: add lazy_interpreter_entry_point() for C2

  // **********
  // * FIXME: * ADD ACCESS TO FLAGS!!!!
  // **********
  // public boolean isInUse();
  // public boolean isNotEntrant();

  // ********************************
  // * MAJOR FIXME: MAJOR HACK HERE *
  // ********************************
  // public boolean isYoung();
  // public boolean isOld();
  // public int     age();
  // public boolean isMarkedForDeoptimization();
  // public boolean isMarkedForUnloading();
  // public int     level();
  // public int     version();

  // FIXME: add mutators for above
  // FIXME: add exception cache access?

  /** On-stack replacement support */
  // FIXME: add mutators
  public int getOSREntryBCI() {
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(getEntryBCI() != VM.getVM().getInvocationEntryBCI(), "wrong kind of nmethod");
    }
    return getEntryBCI();
  }

  public NMethod getOSRLink() {
    return VMObjectFactory.newObject(NMethod.class, osrLinkField.getValue(addr));
  }

  // MethodHandle
  public boolean isMethodHandleReturn(Address returnPc) {
    // Hard to read a bit fields from Java and it's only there for performance
    // so just go directly to the PCDesc
    // if (!hasMethodHandleInvokes())  return false;
    PCDesc pd = getPCDescAt(returnPc);
    if (pd == null)
      return false;
    return pd.isMethodHandleInvoke();
  }

  // Deopt
  // Return true is the PC is one would expect if the frame is being deopted.
  public boolean isDeoptPc      (Address pc) { return isDeoptEntry(pc) || isDeoptMhEntry(pc); }
  public boolean isDeoptEntry   (Address pc) { return pc == deoptHandlerBegin(); }
  public boolean isDeoptMhEntry (Address pc) { return pc == deoptMhHandlerBegin(); }

  /** Tells whether frames described by this nmethod can be
      deoptimized. Note: native wrappers cannot be deoptimized. */
  public boolean canBeDeoptimized() { return isJavaMethod(); }

  // FIXME: add inline cache support
  // FIXME: add flush()

  // FIXME: add mark_as_seen_on_stack
  // FIXME: add can_not_entrant_be_converted

  // FIXME: add GC support
  //  void follow_roots_or_mark_for_unloading(bool unloading_occurred, bool& marked_for_unloading);
  //  void follow_root_or_mark_for_unloading(oop* root, bool unloading_occurred, bool& marked_for_unloading);
  //  void preserve_callee_argument_oops(frame fr, const RegisterMap *reg_map, void f(oop*));
  //  void adjust_pointers();

  /** Finds a PCDesc with real-pc equal to "pc" */
  public PCDesc getPCDescAt(Address pc) {
    // FIXME: consider adding cache like the one down in the VM
    for (Address p = scopesPCsBegin(); p.lessThan(scopesPCsEnd()); p = p.addOffsetTo(pcDescSize)) {
      PCDesc pcDesc = new PCDesc(p);
      if (pcDesc.getRealPC(this).equals(pc)) {
        return pcDesc;
      }
    }
    return null;
  }

  /**
   * Attempt to decode all the debug info in this nmethod.  This is intended purely for testing.
   */
  public void decodeAllScopeDescs() {
    for (Address p = scopesPCsBegin(); p.lessThan(scopesPCsEnd()); p = p.addOffsetTo(pcDescSize)) {
      PCDesc pd = new PCDesc(p);
      if (pd.getPCOffset() == -1) {
        break;
      }
      ScopeDesc sd = new ScopeDesc(this, pd.getScopeDecodeOffset(), pd.getObjDecodeOffset(), pd.getReexecute());
    }
  }

  /** ScopeDesc for an instruction */
  public ScopeDesc getScopeDescAt(Address pc) {
    PCDesc pd = getPCDescAt(pc);
    if (Assert.ASSERTS_ENABLED) {
      Assert.that(pd != null, "scope must be present");
    }
    return new ScopeDesc(this, pd.getScopeDecodeOffset(), pd.getObjDecodeOffset(), pd.getReexecute());
  }

  /** This is only for use by the debugging system, and is only
      intended for use in the topmost frame, where we are not
      guaranteed to be at a PC for which we have a PCDesc. It finds
      the PCDesc with realPC closest to the current PC that has
      a valid scope decode offset. */
  public PCDesc getPCDescNearDbg(Address pc) {
    PCDesc bestGuessPCDesc = null;
    long bestDistance = 0;
    for (Address p = scopesPCsBegin(); p.lessThan(scopesPCsEnd()); p = p.addOffsetTo(pcDescSize)) {
      PCDesc pcDesc = new PCDesc(p);
      if (pcDesc.getScopeDecodeOffset() == DebugInformationRecorder.SERIALIZED_NULL) {
        // We've observed a serialized null decode offset. Ignore this PcDesc.
        continue;
      }
      // In case pc is null
      long distance = -pcDesc.getRealPC(this).minus(pc);
      if ((bestGuessPCDesc == null) ||
          ((distance >= 0) && (distance < bestDistance))) {
        bestGuessPCDesc = pcDesc;
        bestDistance    = distance;
      }
    }
    return bestGuessPCDesc;
  }

  PCDesc find_pc_desc(long pc, boolean approximate) {
    return find_pc_desc_internal(pc, approximate);
  }

  // Finds a PcDesc with real-pc equal to "pc"
  PCDesc find_pc_desc_internal(long pc, boolean approximate) {
    long base_address = VM.getAddressValue(codeBegin());
    int pc_offset = (int) (pc - base_address);

    // Fallback algorithm: quasi-linear search for the PcDesc
    // Find the last pc_offset less than the given offset.
    // The successor must be the required match, if there is a match at all.
    // (Use a fixed radix to avoid expensive affine pointer arithmetic.)
    Address lower = scopesPCsBegin();
    Address upper = scopesPCsEnd();
    upper = upper.addOffsetTo(-pcDescSize); // exclude final sentinel
    if (lower.greaterThan(upper))  return null;  // native method; no PcDescs at all

    // Take giant steps at first (4096, then 256, then 16, then 1)
    int LOG2_RADIX = 4;
    Address mid;
    for (int step = (1 << (LOG2_RADIX*3)); step > 1; step >>= LOG2_RADIX) {
      while ((mid = lower.addOffsetTo(step * pcDescSize)).lessThan(upper)) {
        PCDesc m = new PCDesc(mid);
        if (m.getPCOffset() < pc_offset) {
          lower = mid;
        } else {
          upper = mid;
          break;
        }
      }
    }
    // Sneak up on the value with a linear search of length ~16.
    while (true) {
      mid = lower.addOffsetTo(pcDescSize);
      PCDesc m = new PCDesc(mid);
      if (m.getPCOffset() < pc_offset) {
        lower = mid;
      } else {
        upper = mid;
        break;
      }
    }

    PCDesc u = new PCDesc(upper);
    if (match_desc(u, pc_offset, approximate)) {
      return u;
    } else {
      return null;
    }
  }

  // ScopeDesc retrieval operation
  PCDesc pc_desc_at(long pc)   { return find_pc_desc(pc, false); }
  // pc_desc_near returns the first PCDesc at or after the givne pc.
  PCDesc pc_desc_near(long pc) { return find_pc_desc(pc, true); }

  // Return the last scope in (begin..end]
  public ScopeDesc scope_desc_in(long begin, long end) {
    PCDesc p = pc_desc_near(begin+1);
    if (p != null && VM.getAddressValue(p.getRealPC(this)) <= end) {
      return new ScopeDesc(this, p.getScopeDecodeOffset(), p.getObjDecodeOffset(), p.getReexecute());
    }
    return null;
  }

  static boolean match_desc(PCDesc pc, int pc_offset, boolean approximate) {
    if (!approximate) {
      return pc.getPCOffset() == pc_offset;
    } else {
      PCDesc prev = new PCDesc(pc.getAddress().addOffsetTo(-pcDescSize));
       return prev.getPCOffset() < pc_offset && pc_offset <= pc.getPCOffset();
    }
  }

  /** This is only for use by the debugging system, and is only
      intended for use in the topmost frame, where we are not
      guaranteed to be at a PC for which we have a PCDesc. It finds
      the ScopeDesc closest to the current PC. NOTE that this may
      return NULL for compiled methods which don't have any
      ScopeDescs! */
  public ScopeDesc getScopeDescNearDbg(Address pc) {
    PCDesc pd = getPCDescNearDbg(pc);
    if (pd == null) return null;
    return new ScopeDesc(this, pd.getScopeDecodeOffset(), pd.getObjDecodeOffset(), pd.getReexecute());
  }

  public Map<sun.jvm.hotspot.debugger.Address, PCDesc> getSafepoints() {
    Map<sun.jvm.hotspot.debugger.Address, PCDesc> safepoints = new HashMap<>();
    sun.jvm.hotspot.debugger.Address p = null;
    for (p = scopesPCsBegin(); p.lessThan(scopesPCsEnd());
         p = p.addOffsetTo(pcDescSize)) {
       PCDesc pcDesc = new PCDesc(p);
       sun.jvm.hotspot.debugger.Address pc = pcDesc.getRealPC(this);
       safepoints.put(pc, pcDesc);
    }
    return safepoints;
  }

  // FIXME: add getPCOffsetForBCI()
  // FIXME: add embeddedOopAt()
  // FIXME: add isDependentOn()
  // FIXME: add isPatchableAt()

  /** Support for code generation. Only here for proof-of-concept. */
  public int getEntryPointOffset()            { return (int) entryOffsetField.getValue(addr);        }
  public int getVerifiedEntryPointOffset()    { return (int) verifiedEntryOffsetField.getValue(addr);}
  public static int getOSREntryPointOffset()  { return (int) osrEntryPointField.getOffset();         }
  public static int getEntryBCIOffset()       { return (int) entryBCIField.getOffset();              }
  public static int getMethodOffset()         { return (int) methodField.getOffset();                }

  public void print() {
    printOn(System.out);
  }

  protected void printComponentsOn(PrintStream tty) {
    // FIXME: add relocation information
    tty.println(" content: [" + contentBegin() + ", " + contentEnd() + "), " +
                " code: [" + codeBegin() + ", " + codeEnd() + "), " +
                " data: [" + dataBegin() + ", " + dataEnd() + "), " +
                " oops: [" + oopsBegin() + ", " + oopsEnd() + "), " +
                " frame size: " + getFrameSize());
  }

  public String toString() {
    Method method = getMethod();
    return "NMethod for " +
            method.getMethodHolder().getName().asString() + "." +
            method.getName().asString() + method.getSignature().asString() + "==>n" +
            super.toString();
  }

  public String flagsToString() {
    // FIXME need access to flags...
    return "";
  }

  public String getName() {
    Method method = getMethod();
    return "NMethod for " +
           method.getMethodHolder().getName().asString() + "." +
           method.getName().asString() +
           method.getSignature().asString();
  }

  //--------------------------------------------------------------------------------
  // Internals only below this point
  //

  private int getEntryBCI()           { return (int) entryBCIField          .getValue(addr); }
  private int getExceptionOffset()    { return (int) exceptionOffsetField   .getValue(addr); }
  private int getDeoptHandlerOffset()   { return (int) deoptHandlerOffsetField  .getValue(addr); }
  private int getDeoptMhHandlerOffset() { return (int) deoptMhHandlerOffsetField.getValue(addr); }
  private int getStubOffset()         { return (int) stubOffsetField        .getValue(addr); }
  private int getScopesDataOffset()   { return (int) scopesDataOffsetField  .getValue(addr); }
  private int getScopesPCsOffset()    { return (int) scopesPCsOffsetField   .getValue(addr); }
  private int getHandlerTableOffset() { return (int) handlerTableOffsetField.getValue(addr); }
  private int getNulChkTableOffset()  { return (int) nulChkTableOffsetField .getValue(addr); }
  private int getCompLevel()          { return (int) compLevelField         .getValue(addr); }
}
