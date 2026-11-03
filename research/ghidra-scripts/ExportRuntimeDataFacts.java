// Read-only literal and virtual-table checks for the runtime study.
// @category Elements of A
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class ExportRuntimeDataFacts extends GhidraScript {
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        try (BufferedWriter out = Files.newBufferedWriter(Path.of(args[0]), StandardCharsets.UTF_8)) {
            out.write("SHA256: " + currentProgram.getExecutableSHA256() + "\n");
            Address literal = toAddr("00105abc");
            out.write("Tick multiplier at " + literal + ": 0x" + Integer.toHexString(getInt(literal)) + "\n");
            for (String entry : new String[]{"001049f8", "00105b9c", "000a4e68", "002e7ba8"}) {
                Function fn = getFunctionAt(toAddr(entry));
                if (fn == null) continue;
                Function target = fn.isThunk() ? fn.getThunkedFunction(true) : fn;
                out.write("\nINSTRUCTIONS " + fn.getName(true) + " " + fn.getEntryPoint()
                    + " actual " + target.getEntryPoint() + "\n");
                InstructionIterator ins = currentProgram.getListing().getInstructions(target.getBody(), true);
                while (ins.hasNext()) {
                    Instruction instruction = ins.next();
                    out.write(instruction.getAddress() + " " + instruction + "\n");
                    for (Address ref : instruction.getFlows()) {
                        Function called = getFunctionAt(ref);
                        if (called != null) out.write("  flow " + called.getName(true) + "\n");
                    }
                }
            }
            SymbolIterator symbols = currentProgram.getSymbolTable().getSymbols("vtable");
            while (symbols.hasNext()) {
                Symbol symbol = symbols.next();
                if (!symbol.getName(true).contains("Gfx::OpenGlRenderQueue")) continue;
                out.write("\nVTABLE " + symbol.getName(true) + " " + symbol.getAddress() + "\n");
                for (int offset = 0; offset < 48; offset += 4) {
                    int value = getInt(symbol.getAddress().add(offset));
                    Address candidate = toAddr(Integer.toUnsignedLong(value & ~1));
                    Function function = getFunctionAt(candidate);
                    out.write("  +" + offset + " 0x" + Integer.toHexString(value)
                        + (function == null ? "" : " " + function.getName(true)) + "\n");
                }
            }
        }
        println("Wrote runtime data facts to " + args[0]);
    }
}
