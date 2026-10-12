// Inventory non-protected runtime entry points by namespace and function name.
// @category Elements of A

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class InventoryRuntimeSymbols extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("Expected output TSV path");
        String[] include = {"Luna::", "LunaSdk::", "Luna_", "Gfx::", "Ui::", "Input::",
            "JavaScript", "Javascript", "V8::", "Node::", "NodeJs::", "cw_v8",
            "Widget::", "Window::", "Font::", "Text::", "Image::", "Animation::",
            "Layout::", "Event::", "Timer::", "Media::", "Network::", "Platform::"};
        String[] exclude = {"drm", "license", "decrypt", "crypto", "cdm", "fairplay"};
        int count = 0;
        try (BufferedWriter out = Files.newBufferedWriter(Path.of(args[0]), StandardCharsets.UTF_8)) {
            out.write("address\tname\tbytes\n");
            FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                String name = function.getName(true);
                boolean relevant = false;
                for (String prefix : include) if (name.contains(prefix)) relevant = true;
                if (!relevant) continue;
                String lower = name.toLowerCase();
                boolean protectedPath = false;
                for (String word : exclude) if (lower.contains(word)) protectedPath = true;
                if (protectedPath) continue;
                out.write(function.getEntryPoint() + "\t" + name.replace('\t', ' ') + "\t"
                    + function.getBody().getNumAddresses() + "\n");
                count++;
            }
        }
        println("Wrote " + count + " runtime functions to " + args[0]);
    }
}
