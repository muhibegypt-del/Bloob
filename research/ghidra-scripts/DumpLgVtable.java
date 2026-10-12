// Dump only the LG playback adapter's virtual method table.
// @category Elements of A

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class DumpLgVtable extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Output path required");
        Address table = currentProgram.getAddressFactory().getAddress("01208d4c");
        try (BufferedWriter writer = Files.newBufferedWriter(Path.of(args[0]), StandardCharsets.UTF_8)) {
            writer.write("table_offset\tvirtual_offset\tpointer\tfunction\n");
            for (int offset = 0; offset <= 0x90; offset += 4) {
                Address slot = table.add(offset);
                long pointer = Integer.toUnsignedLong(getInt(slot));
                Address target = currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(pointer);
                Function function = currentProgram.getFunctionManager().getFunctionAt(target);
                String name = function == null ? "" : function.getName(true);
                writer.write(String.format("0x%02x\t%s\t%s\t%s%n", offset,
                    offset < 8 ? "header" : String.format("0x%02x", offset - 8), target, name));
            }
        }
        println("Wrote LG adapter vtable entries");
    }
}
