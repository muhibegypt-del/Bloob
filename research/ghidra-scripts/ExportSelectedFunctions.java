// Export only explicitly selected native functions for the playback architecture map.
// First script argument is an output path; remaining arguments are hex entry points.
// @category Elements of A

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class ExportSelectedFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("Output path and addresses required");
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try (BufferedWriter writer = Files.newBufferedWriter(Path.of(args[0]), StandardCharsets.UTF_8)) {
            for (int i = 1; i < args.length && !monitor.isCancelled(); i++) {
                Address entry = currentProgram.getAddressFactory().getAddress(args[i]);
                Function function = currentProgram.getFunctionManager().getFunctionAt(entry);
                if (function == null) {
                    writer.write("\n=== " + args[i] + " no function ===\n");
                    continue;
                }
                writer.write("\n=== " + args[i] + " " + function.getName(true)
                    + " bytes=" + function.getBody().getNumAddresses() + " ===\n");
                DecompileResults result = decompiler.decompileFunction(function, 90, monitor);
                if (!result.decompileCompleted() || result.getDecompiledFunction() == null) {
                    writer.write("DECOMPILATION FAILED: " + result.getErrorMessage() + "\n");
                    continue;
                }
                for (String line : result.getDecompiledFunction().getC().split("\n", -1)) {
                    String lower = line.toLowerCase();
                    if (lower.contains("drm") || lower.contains("license")
                            || lower.contains("decrypt") || lower.contains("crypto")
                            || lower.contains("cdm")) {
                        writer.write("[excluded protected-content line]\n");
                    } else {
                        writer.write(line + "\n");
                    }
                }
            }
        } finally {
            decompiler.dispose();
        }
        println("Wrote " + (args.length - 1) + " selected functions to " + args[0]);
    }
}
