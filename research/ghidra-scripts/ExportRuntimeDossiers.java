// Read-only focused native runtime evidence. Raw output stays local.
// @category Elements of A
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;

public class ExportRuntimeDossiers extends GhidraScript {
    private boolean excluded(String value) {
        String lower = value.toLowerCase();
        for (String word : new String[]{"drm", "license", "decrypt", "crypto", "cdm", "fairplay"})
            if (lower.contains(word)) return true;
        return false;
    }
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("Output path and entry addresses required");
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        int completed = 0;
        try (BufferedWriter out = Files.newBufferedWriter(Path.of(args[0]), StandardCharsets.UTF_8)) {
            out.write("Program SHA256: " + currentProgram.getExecutableSHA256() + "\n");
            out.write("Language: " + currentProgram.getLanguageID() + "\n");
            out.write("Static Ghidra observations; direct call sets do not resolve all indirect dispatch.\n");
            for (int i = 1; i < args.length && !monitor.isCancelled(); i++) {
                Function fn = currentProgram.getFunctionManager().getFunctionAt(toAddr(args[i]));
                if (fn == null) { out.write("\n=== " + args[i] + " NO FUNCTION ===\n"); continue; }
                if (excluded(fn.getName(true))) continue;
                out.write("\n=== " + fn.getEntryPoint() + " " + fn.getName(true)
                    + " bytes=" + fn.getBody().getNumAddresses() + " ===\n");
                ArrayList<Function> callers = new ArrayList<>(fn.getCallingFunctions(monitor));
                ArrayList<Function> callees = new ArrayList<>(fn.getCalledFunctions(monitor));
                callers.sort(Comparator.comparing(f -> f.getEntryPoint().toString()));
                callees.sort(Comparator.comparing(f -> f.getEntryPoint().toString()));
                out.write("CALLERS (resolved static):\n");
                for (Function caller : callers) if (!excluded(caller.getName(true)))
                    out.write("  " + caller.getEntryPoint() + " " + caller.getName(true) + "\n");
                out.write("CALLEES (resolved static):\n");
                for (Function callee : callees) if (!excluded(callee.getName(true)))
                    out.write("  " + callee.getEntryPoint() + " " + callee.getName(true) + "\n");
                DecompileResults result = decompiler.decompileFunction(fn, 90, monitor);
                out.write("PSEUDOCODE:\n");
                if (!result.decompileCompleted() || result.getDecompiledFunction() == null) {
                    out.write("DECOMPILATION FAILED: " + result.getErrorMessage() + "\n");
                    continue;
                }
                for (String line : result.getDecompiledFunction().getC().split("\n", -1))
                    out.write(excluded(line) ? "[excluded protected-content line]\n" : line + "\n");
                completed++;
            }
        } finally { decompiler.dispose(); }
        println("Exported " + completed + " runtime dossiers to " + args[0]);
    }
}
