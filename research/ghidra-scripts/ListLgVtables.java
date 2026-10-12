// Locate LG playback-adapter vtable labels for indirect-call verification.
// @category Elements of A

import ghidra.app.script.GhidraScript;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class ListLgVtables extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Output path required");
        int count = 0;
        try (BufferedWriter writer = Files.newBufferedWriter(Path.of(args[0]), StandardCharsets.UTF_8)) {
            SymbolIterator symbols = currentProgram.getSymbolTable().getAllSymbols(true);
            while (symbols.hasNext() && !monitor.isCancelled()) {
                Symbol symbol = symbols.next();
                String name = symbol.getName(true);
                if (name.contains("LgEsPlayerPalImpl") &&
                    (name.toLowerCase().contains("vtable") || name.contains("_ZTV"))) {
                    writer.write(symbol.getAddress() + "\t" + name + "\n");
                    count++;
                }
            }
        }
        println("Found " + count + " matching vtable labels");
    }
}
