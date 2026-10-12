// Probe whether a symbol-only import can be decompiled without full autoanalysis.
// @category Elements of A

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class ProbeTargetedDisassembly extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address entry = currentProgram.getAddressFactory().getAddress("000cb8c4");
        Function before = currentProgram.getFunctionManager().getFunctionAt(entry);
        println("Before body bytes: " + (before == null ? "none" : before.getBody().getNumAddresses()));
        boolean changed = disassemble(entry);
        Function after = currentProgram.getFunctionManager().getFunctionAt(entry);
        println("Disassembled: " + changed);
        println("After body bytes: " + (after == null ? "none" : after.getBody().getNumAddresses()));
        if (after == null) return;
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try {
            DecompileResults result = decompiler.decompileFunction(after, 30, monitor);
            println("Decompiler completed: " + result.decompileCompleted());
            if (!result.decompileCompleted()) println(result.getErrorMessage());
        } finally {
            decompiler.dispose();
        }
    }
}
