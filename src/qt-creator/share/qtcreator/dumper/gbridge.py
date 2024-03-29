
import base64
import binascii
import __builtin__
import gdb
import inspect
import os
import os.path
import subprocess
import sys
import tempfile
import traceback


cdbLoaded = False
lldbLoaded = False
gdbLoaded = True

#######################################################################
#
# Infrastructure
#
#######################################################################

def warn(message):
    print "XXX: %s\n" % message.encode("latin1")

def directBaseClass(typeobj, index = 0):
    # FIXME: Check it's really a base.
    return typeobj.fields()[index]

def savePrint(output):
    try:
        print(output)
    except:
        out = ""
        for c in output:
            cc = ord(c)
            if cc > 127:
                out += "\\\\%d" % cc
            elif cc < 0:
                out += "\\\\%d" % (cc + 256)
            else:
                out += c
        print(out)

def registerCommand(name, func):

    class Command(gdb.Command):
        def __init__(self):
            super(Command, self).__init__(name, gdb.COMMAND_OBSCURE)
        def invoke(self, args, from_tty):
            savePrint(func(args))

    Command()

def parseAndEvaluate(exp):
    return gdb.parse_and_eval(exp)

def extractFields(value):
    return value.type.fields()
    ## Insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=10953:
    ##fields = type.fields()
    ## Insufficient, see http://sourceware.org/bugzilla/show_bug.cgi?id=11777:
    ##fields = defsype).fields()
    ## This seems to work.
    ##warn("TYPE 0: %s" % type)
    #type = stripTypedefs(type)
    #fields = type.fields()
    #if len(fields):
    #    return fields
    ##warn("TYPE 1: %s" % type)
    ## This fails for arrays. See comment in lookupType.
    #type0 = lookupType(str(type))
    #if not type0 is None:
    #    type = type0
    #if type.code == FunctionCode:
    #    return []
    ##warn("TYPE 2: %s" % type)
    #fields = type.fields()
    ##warn("FIELDS: %s" % fields)
    #return fields

def fieldCount(type):
    return len(type.fields())

def listOfLocals(varList):
    frame = gdb.selected_frame()
    try:
        frame = gdb.selected_frame()
        #warn("FRAME %s: " % frame)
    except RuntimeError, error:
        warn("FRAME NOT ACCESSIBLE: %s" % error)
        return []
    except:
        warn("FRAME NOT ACCESSIBLE FOR UNKNOWN REASONS")
        return []

    try:
        block = frame.block()
        #warn("BLOCK: %s " % block)
    except RuntimeError, error:
        #warn("BLOCK IN FRAME NOT ACCESSIBLE: %s" % error)
        return []
    except:
        warn("BLOCK NOT ACCESSIBLE FOR UNKNOWN REASONS")
        return []

    items = []
    shadowed = {}
    while True:
        if block is None:
            warn("UNEXPECTED 'None' BLOCK")
            break
        for symbol in block:
            name = symbol.print_name

            if name == "__in_chrg" or name == "__PRETTY_FUNCTION__":
                continue

            # "NotImplementedError: Symbol type not yet supported in
            # Python scripts."
            #warn("SYMBOL %s  (%s): " % (symbol, name))
            if name in shadowed:
                level = shadowed[name]
                name1 = "%s@%s" % (name, level)
                shadowed[name] = level + 1
            else:
                name1 = name
                shadowed[name] = 1
            #warn("SYMBOL %s  (%s, %s)): " % (symbol, name, symbol.name))
            item = LocalItem()
            item.iname = "local." + name1
            item.name = name1
            try:
                item.value = frame.read_var(name, block)
                #warn("READ 1: %s" % item.value)
                if not item.value.is_optimized_out:
                    #warn("ITEM 1: %s" % item.value)
                    items.append(item)
                    continue
            except:
                pass

            try:
                item.value = frame.read_var(name)
                #warn("READ 2: %s" % item.value)
                if not item.value.is_optimized_out:
                    #warn("ITEM 2: %s" % item.value)
                    items.append(item)
                    continue
            except:
                # RuntimeError: happens for
                #     void foo() { std::string s; std::wstring w; }
                # ValueError: happens for (as of 2010/11/4)
                #     a local struct as found e.g. in
                #     gcc sources in gcc.c, int execute()
                pass

            try:
                #warn("READ 3: %s %s" % (name, item.value))
                item.value = gdb.parse_and_eval(name)
                #warn("ITEM 3: %s" % item.value)
                items.append(item)
            except:
                # Can happen in inlined code (see last line of
                # RowPainter::paintChars(): "RuntimeError:
                # No symbol \"__val\" in current context.\n"
                pass

        # The outermost block in a function has the function member
        # FIXME: check whether this is guaranteed.
        if not block.function is None:
            break

        block = block.superblock

    return items


def catchCliOutput(command):
    try:
        return gdb.execute(command, to_string=True).split("\n")
    except:
        pass
    filename = createTempFile()
    gdb.execute("set logging off")
#        gdb.execute("set logging redirect off")
    gdb.execute("set logging file %s" % filename)
#        gdb.execute("set logging redirect on")
    gdb.execute("set logging on")
    msg = ""
    try:
        gdb.execute(command)
    except RuntimeError, error:
        # For the first phase of core file loading this yield
        # "No symbol table is loaded.  Use the \"file\" command."
        msg = str(error)
    except:
        msg = "Unknown error"
    gdb.execute("set logging off")
#        gdb.execute("set logging redirect off")
    if len(msg):
        # Having that might confuse result handlers in the gdbengine.
        #warn("CLI ERROR: %s " % msg)
        removeTempFile(filename)
        return "CLI ERROR: %s " % msg
    temp = open(filename, "r")
    lines = []
    for line in temp:
        lines.append(line)
    temp.close()
    removeTempFile(filename)
    return lines

def selectedInferior():
    try:
        # Does not exist in 7.3.
        return gdb.selected_inferior()
    except:
        pass
    # gdb.Inferior is new in gdb 7.2
    return gdb.inferiors()[0]

def readRawMemory(base, size):
    try:
        inferior = selectedInferior()
        return binascii.hexlify(inferior.read_memory(base, size))
    except:
        pass
    s = ""
    t = lookupType("unsigned char").pointer()
    base = gdb.Value(base).cast(t)
    for i in xrange(size):
        s += "%02x" % int(base.dereference())
        base += 1
    return s


#######################################################################
#
# Types
#
#######################################################################

PointerCode = gdb.TYPE_CODE_PTR
ArrayCode = gdb.TYPE_CODE_ARRAY
StructCode = gdb.TYPE_CODE_STRUCT
UnionCode = gdb.TYPE_CODE_UNION
EnumCode = gdb.TYPE_CODE_ENUM
FlagsCode = gdb.TYPE_CODE_FLAGS
FunctionCode = gdb.TYPE_CODE_FUNC
IntCode = gdb.TYPE_CODE_INT
FloatCode = gdb.TYPE_CODE_FLT # Parts of GDB assume that this means complex.
VoidCode = gdb.TYPE_CODE_VOID
#SetCode = gdb.TYPE_CODE_SET
RangeCode = gdb.TYPE_CODE_RANGE
StringCode = gdb.TYPE_CODE_STRING
#BitStringCode = gdb.TYPE_CODE_BITSTRING
#ErrorTypeCode = gdb.TYPE_CODE_ERROR
MethodCode = gdb.TYPE_CODE_METHOD
MethodPointerCode = gdb.TYPE_CODE_METHODPTR
MemberPointerCode = gdb.TYPE_CODE_MEMBERPTR
ReferenceCode = gdb.TYPE_CODE_REF
CharCode = gdb.TYPE_CODE_CHAR
BoolCode = gdb.TYPE_CODE_BOOL
ComplexCode = gdb.TYPE_CODE_COMPLEX
TypedefCode = gdb.TYPE_CODE_TYPEDEF
NamespaceCode = gdb.TYPE_CODE_NAMESPACE
#Code = gdb.TYPE_CODE_DECFLOAT # Decimal floating point.
#Code = gdb.TYPE_CODE_MODULE # Fortran
#Code = gdb.TYPE_CODE_INTERNAL_FUNCTION
SimpleValueCode = -1


#######################################################################
#
# Step Command
#
#######################################################################

def sal(args):
    (cmd, addr) = args.split(",")
    lines = catchCliOutput("info line *" + addr)
    fromAddr = "0x0"
    toAddr = "0x0"
    for line in lines:
        pos0from = line.find(" starts at address") + 19
        pos1from = line.find(" ", pos0from)
        pos0to = line.find(" ends at", pos1from) + 9
        pos1to = line.find(" ", pos0to)
        if pos1to > 0:
            fromAddr = line[pos0from : pos1from]
            toAddr = line[pos0to : pos1to]
    gdb.execute("maint packet sal%s,%s,%s" % (cmd,fromAddr, toAddr))

registerCommand("sal", sal)


#######################################################################
#
# Convenience
#
#######################################################################

# Just convienience for 'python print ...'
class PPCommand(gdb.Command):
    def __init__(self):
        super(PPCommand, self).__init__("pp", gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        print(eval(args))

PPCommand()

# Just convienience for 'python print gdb.parse_and_eval(...)'
class PPPCommand(gdb.Command):
    def __init__(self):
        super(PPPCommand, self).__init__("ppp", gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        print(gdb.parse_and_eval(args))

PPPCommand()


def scanStack(p, n):
    p = long(p)
    r = []
    for i in xrange(n):
        f = gdb.parse_and_eval("{void*}%s" % p)
        m = gdb.execute("info symbol %s" % f, to_string=True)
        if not m.startswith("No symbol matches"):
            r.append(m)
        p += f.type.sizeof
    return r

class ScanStackCommand(gdb.Command):
    def __init__(self):
        super(ScanStackCommand, self).__init__("scanStack", gdb.COMMAND_OBSCURE)
    def invoke(self, args, from_tty):
        if len(args) == 0:
            args = 20
        savePrint(scanStack(gdb.parse_and_eval("$sp"), int(args)))

ScanStackCommand()

# This is a cache mapping from 'type name' to 'display alternatives'.
qqFormats = {}

# This is a cache of all known dumpers.
qqDumpers = {}

# This is a cache of all dumpers that support writing.
qqEditable = {}

# This is an approximation of the Qt Version found
qqVersion = None

def registerDumper(function):
    global qqDumpers, qqFormats, qqEditable
    try:
        funcname = function.func_name
        if funcname.startswith("qdump__"):
            type = funcname[7:]
            qqDumpers[type] = function
            qqFormats[type] = qqFormats.get(type, "")
        elif funcname.startswith("qform__"):
            type = funcname[7:]
            formats = ""
            try:
                formats = function()
            except:
                pass
            qqFormats[type] = formats
        elif funcname.startswith("qedit__"):
            type = funcname[7:]
            try:
                qqEditable[type] = function
            except:
                pass
    except:
        pass

def bbsetup(args = ''):
    global qqDumpers, qqFormats, qqEditable, typeCache
    qqDumpers = {}
    qqFormats = {}
    qqEditable = {}
    typeCache = {}
    module = sys.modules[__name__]

    for key, value in module.__dict__.items():
        registerDumper(value)

    result = "dumpers=["
    #qqNs = qtNamespace() # This is too early
    for key, value in qqFormats.items():
        if qqEditable.has_key(key):
            result += '{type="%s",formats="%s",editable="true"},' % (key, value)
        else:
            result += '{type="%s",formats="%s"},' % (key, value)
    result += ']'
    #result += ',namespace="%s"' % qqNs
    print result
    return result

registerCommand("bbsetup", bbsetup)


#######################################################################
#
# Import plain gdb pretty printers
#
#######################################################################

class PlainDumper:
    def __init__(self, printer):
        self.printer = printer

    def __call__(self, d, value):
        printer = self.printer.gen_printer(value)
        lister = getattr(printer, "children", None)
        children = [] if lister is None else list(lister())
        d.putType(self.printer.name)
        val = printer.to_string().encode("hex")
        d.putValue(val, Hex2EncodedLatin1)
        d.putValue(printer.to_string())
        d.putNumChild(len(children))
        if d.isExpanded():
            with Children(d):
                for child in children:
                    d.putSubItem(child[0], child[1])

def importPlainDumper(printer):
    global qqDumpers, qqFormats
    name = printer.name.replace("::", "__")
    qqDumpers[name] = PlainDumper(printer)
    qqFormats[name] = ""

def importPlainDumpers(args):
    return
    for obj in gdb.objfiles():
        for printers in obj.pretty_printers + gdb.pretty_printers:
            for printer in printers.subprinters:
                importPlainDumper(printer)

registerCommand("importPlainDumpers", importPlainDumpers)




# Fails on Windows.
try:
    import curses.ascii
    def printableChar(ucs):
        if curses.ascii.isprint(ucs):
            return ucs
        return '?'
except:
    def printableChar(ucs):
        if ucs >= 32 and ucs <= 126:
            return ucs
        return '?'


def childAt(value, index):
    field = value.type.fields()[index]
    if len(field.name):
        return value[field.name]
    # FIXME: Cheat. There seems to be no official way to access
    # the real item, so we pass back the value. That at least
    # enables later ...["name"] style accesses as gdb handles
    # them transparently.
    return value

def fieldAt(type, index):
    return type.fields()[index]


#gdb.Value.child = impl_Value_child

# Fails on SimulatorQt.
tempFileCounter = 0
try:
    # Test if 2.6 is used (Windows), trigger exception and default
    # to 2nd version.
    file = tempfile.NamedTemporaryFile(prefix="py_",delete=True)
    file.close()
    def createTempFile():
        file = tempfile.NamedTemporaryFile(prefix="py_",delete=True)
        file.close()
        return file.name

except:
    def createTempFile():
        global tempFileCounter
        tempFileCounter += 1
        fileName = "%s/py_tmp_%d_%d" \
            % (tempfile.gettempdir(), os.getpid(), tempFileCounter)
        return fileName

def removeTempFile(name):
    try:
        os.remove(name)
    except:
        pass

def showException(msg, exType, exValue, exTraceback):
    warn("**** CAUGHT EXCEPTION: %s ****" % msg)
    try:
        import traceback
        for line in traceback.format_exception(exType, exValue, exTraceback):
            warn("%s" % line)
    except:
        pass

verbosity = 0
verbosity = 1

# Some "Enums"

# Encodings. Keep that synchronized with DebuggerEncoding in watchutils.h
Unencoded8Bit, \
Base64Encoded8BitWithQuotes, \
Base64Encoded16BitWithQuotes, \
Base64Encoded32BitWithQuotes, \
Base64Encoded16Bit, \
Base64Encoded8Bit, \
Hex2EncodedLatin1, \
Hex4EncodedLittleEndian, \
Hex8EncodedLittleEndian, \
Hex2EncodedUtf8, \
Hex8EncodedBigEndian, \
Hex4EncodedBigEndian, \
Hex4EncodedLittleEndianWithoutQuotes, \
Hex2EncodedLocal8Bit, \
JulianDate, \
MillisecondsSinceMidnight, \
JulianDateAndMillisecondsSinceMidnight, \
Hex2EncodedInt1, \
Hex2EncodedInt2, \
Hex2EncodedInt4, \
Hex2EncodedInt8, \
Hex2EncodedUInt1, \
Hex2EncodedUInt2, \
Hex2EncodedUInt4, \
Hex2EncodedUInt8, \
Hex2EncodedFloat4, \
Hex2EncodedFloat8 \
    = range(27)

# Display modes. Keep that synchronized with DebuggerDisplay in watchutils.h
StopDisplay, \
DisplayImageData, \
DisplayUtf16String, \
DisplayImageFile, \
DisplayProcess, \
DisplayLatin1String, \
DisplayUtf8String \
    = range(7)


qqStringCutOff = 10000

#
# Gnuplot based display for array-like structures.
#
gnuplotPipe = {}
gnuplotPid = {}

def hasPlot():
    fileName = "/usr/bin/gnuplot"
    return os.path.isfile(fileName) and os.access(fileName, os.X_OK)


#
# VTable
#
def hasVTable(type):
    fields = type.fields()
    if len(fields) == 0:
        return False
    if fields[0].is_base_class:
        return hasVTable(fields[0].type)
    return str(fields[0].type) ==  "int (**)(void)"

def dynamicTypeName(value):
    if hasVTable(value.type):
        #vtbl = str(parseAndEvaluate("{int(*)(int)}%s" % long(value.address)))
        try:
            # Fails on 7.1 due to the missing to_string.
            vtbl = gdb.execute("info symbol {int*}%s" % long(value.address),
                to_string = True)
            pos1 = vtbl.find("vtable ")
            if pos1 != -1:
                pos1 += 11
                pos2 = vtbl.find(" +", pos1)
                if pos2 != -1:
                    return vtbl[pos1 : pos2]
        except:
            pass
    return str(value.type)

def downcast(value):
    try:
        return value.cast(value.dynamic_type)
    except:
        pass
    #try:
    #    return value.cast(lookupType(dynamicTypeName(value)))
    #except:
    #    pass
    return value

def expensiveDowncast(value):
    try:
        return value.cast(value.dynamic_type)
    except:
        pass
    try:
        return value.cast(lookupType(dynamicTypeName(value)))
    except:
        pass
    return value

typeCache = {}

def lookupType(typestring):
    global typeCache
    global typesToReport
    type = typeCache.get(typestring)
    #warn("LOOKUP 1: %s -> %s" % (typestring, type))
    if not type is None:
        return type

    if typestring == "void":
        type = gdb.lookup_type(typestring)
        typeCache[typestring] = type
        typesToReport[typestring] = type
        return type

    try:
        type = gdb.parse_and_eval("{%s}&main" % typestring).type
        if not type is None:
            typeCache[typestring] = type
            typesToReport[typestring] = type
            return type
    except:
        pass

    # See http://sourceware.org/bugzilla/show_bug.cgi?id=13269
    # gcc produces "{anonymous}", gdb "(anonymous namespace)"
    # "<unnamed>" has been seen too. The only thing gdb
    # understands when reading things back is "(anonymous namespace)"
    if typestring.find("{anonymous}") != -1:
        ts = typestring
        ts = ts.replace("{anonymous}", "(anonymous namespace)")
        type = lookupType(ts)
        if not type is None:
            typeCache[typestring] = type
            typesToReport[typestring] = type
            return type

    #warn(" RESULT FOR 7.2: '%s': %s" % (typestring, type))

    # This part should only trigger for
    # gdb 7.1 for types with namespace separators.
    # And anonymous namespaces.

    ts = typestring
    while True:
        #warn("TS: '%s'" % ts)
        if ts.startswith("class "):
            ts = ts[6:]
        elif ts.startswith("struct "):
            ts = ts[7:]
        elif ts.startswith("const "):
            ts = ts[6:]
        elif ts.startswith("volatile "):
            ts = ts[9:]
        elif ts.startswith("enum "):
            ts = ts[5:]
        elif ts.endswith(" const"):
            ts = ts[:-6]
        elif ts.endswith(" volatile"):
            ts = ts[:-9]
        elif ts.endswith("*const"):
            ts = ts[:-5]
        elif ts.endswith("*volatile"):
            ts = ts[:-8]
        else:
            break

    if ts.endswith('*'):
        type = lookupType(ts[0:-1])
        if not type is None:
            type = type.pointer()
            typeCache[typestring] = type
            typesToReport[typestring] = type
            return type

    try:
        #warn("LOOKING UP '%s'" % ts)
        type = gdb.lookup_type(ts)
    except RuntimeError, error:
        #warn("LOOKING UP '%s': %s" % (ts, error))
        # See http://sourceware.org/bugzilla/show_bug.cgi?id=11912
        exp = "(class '%s'*)0" % ts
        try:
            type = parseAndEvaluate(exp).type.target()
        except:
            # Can throw "RuntimeError: No type named class Foo."
            pass
    except:
        #warn("LOOKING UP '%s' FAILED" % ts)
        pass

    if not type is None:
        typeCache[typestring] = type
        typesToReport[typestring] = type
        return type

    # This could still be None as gdb.lookup_type("char[3]") generates
    # "RuntimeError: No type named char[3]"
    typeCache[typestring] = type
    typesToReport[typestring] = type
    return type

def cleanAddress(addr):
    if addr is None:
        return "<no address>"
    # We cannot use str(addr) as it yields rubbish for char pointers
    # that might trigger Unicode encoding errors.
    #return addr.cast(lookupType("void").pointer())
    # We do not use "hex(...)" as it (sometimes?) adds a "L" suffix.
    return "0x%x" % long(addr)

def extractTemplateArgument(type, position):
    level = 0
    skipSpace = False
    inner = ""
    type = str(type)
    for c in type[type.find('<') + 1 : -1]:
        if c == '<':
            inner += c
            level += 1
        elif c == '>':
            level -= 1
            inner += c
        elif c == ',':
            if level == 0:
                if position == 0:
                    return inner
                position -= 1
                inner = ""
            else:
                inner += c
                skipSpace = True
        else:
            if skipSpace and c == ' ':
                pass
            else:
                inner += c
                skipSpace = False
    return inner

def templateArgument(type, position):
    try:
        # This fails on stock 7.2 with
        # "RuntimeError: No type named myns::QObject.\n"
        return type.template_argument(position)
    except:
        # That's something like "myns::QList<...>"
        return lookupType(extractTemplateArgument(type.strip_typedefs(), position))


# Workaround for gdb < 7.1
def numericTemplateArgument(type, position):
    try:
        return int(type.template_argument(position))
    except RuntimeError, error:
        # ": No type named 30."
        msg = str(error)
        msg = msg[14:-1]
        # gdb at least until 7.4 produces for std::array<int, 4u>
        # for template_argument(1): RuntimeError: No type named 4u.
        if msg[-1] == 'u':
           msg = msg[0:-1]
        return int(msg)


class OutputSafer:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.savedOutput = self.d.output
        self.d.output = []

    def __exit__(self, exType, exValue, exTraceBack):
        if self.d.passExceptions and not exType is None:
            showException("OUTPUTSAFER", exType, exValue, exTraceBack)
            self.d.output = self.savedOutput
        else:
            self.savedOutput.extend(self.d.output)
            self.d.output = self.savedOutput
        return False


class NoAddress:
    def __init__(self, d):
        self.d = d

    def __enter__(self):
        self.savedPrintsAddress = self.d.currentPrintsAddress
        self.d.currentPrintsAddress = False

    def __exit__(self, exType, exValue, exTraceBack):
        self.d.currentPrintsAddress = self.savedPrintsAddress



class SubItem:
    def __init__(self, d, component):
        self.d = d
        self.iname = "%s.%s" % (d.currentIName, component)
        self.name = component

    def __enter__(self):
        self.d.put('{')
        #if not self.name is None:
        if isinstance(self.name, str):
            self.d.put('name="%s",' % self.name)
        self.savedIName = self.d.currentIName
        self.savedValue = self.d.currentValue
        self.savedValuePriority = self.d.currentValuePriority
        self.savedValueEncoding = self.d.currentValueEncoding
        self.savedType = self.d.currentType
        self.savedTypePriority = self.d.currentTypePriority
        self.savedCurrentAddress = self.d.currentAddress
        self.d.currentIName = self.iname
        self.d.currentValuePriority = -100
        self.d.currentValueEncoding = None
        self.d.currentType = ""
        self.d.currentTypePriority = -100
        self.d.currentAddress = None

    def __exit__(self, exType, exValue, exTraceBack):
        #warn(" CURRENT VALUE: %s %s %s" % (self.d.currentValue,
        #    self.d.currentValueEncoding, self.d.currentValuePriority))
        if not exType is None:
            if self.d.passExceptions:
                showException("SUBITEM", exType, exValue, exTraceBack)
            self.d.putNumChild(0)
            self.d.putValue("<not accessible>")
        try:
            #warn("TYPE VALUE: %s" % self.d.currentValue)
            typeName = stripClassTag(self.d.currentType)
            #warn("TYPE: '%s'  DEFAULT: '%s' % (typeName, self.d.currentChildType))

            if len(typeName) > 0 and typeName != self.d.currentChildType:
                self.d.put('type="%s",' % typeName) # str(type.unqualified()) ?
            if  self.d.currentValue is None:
                self.d.put('value="<not accessible>",numchild="0",')
            else:
                if not self.d.currentValueEncoding is None:
                    self.d.put('valueencoded="%d",' % self.d.currentValueEncoding)
                self.d.put('value="%s",' % self.d.currentValue)
        except:
            pass
        if not self.d.currentAddress is None:
            self.d.put(self.d.currentAddress)
        self.d.put('},')
        self.d.currentIName = self.savedIName
        self.d.currentValue = self.savedValue
        self.d.currentValuePriority = self.savedValuePriority
        self.d.currentValueEncoding = self.savedValueEncoding
        self.d.currentType = self.savedType
        self.d.currentTypePriority = self.savedTypePriority
        self.d.currentAddress = self.savedCurrentAddress
        return True

class TopLevelItem(SubItem):
    def __init__(self, d, iname):
        self.d = d
        self.iname = iname
        self.name = None

class UnnamedSubItem(SubItem):
    def __init__(self, d, component):
        self.d = d
        self.iname = "%s.%s" % (self.d.currentIName, component)
        self.name = None

class Children:
    def __init__(self, d, numChild = 1, childType = None, childNumChild = None,
            maxNumChild = None, addrBase = None, addrStep = None):
        self.d = d
        self.numChild = numChild
        self.childNumChild = childNumChild
        self.maxNumChild = maxNumChild
        self.addrBase = addrBase
        self.addrStep = addrStep
        self.printsAddress = True
        if childType is None:
            self.childType = None
        else:
            self.childType = stripClassTag(str(childType))
            self.d.put('childtype="%s",' % self.childType)
            if childNumChild is None:
                if isSimpleType(childType):
                    self.d.put('childnumchild="0",')
                    self.childNumChild = 0
                elif childType.code == PointerCode:
                    self.d.put('childnumchild="1",')
                    self.childNumChild = 1
            else:
                self.d.put('childnumchild="%s",' % childNumChild)
                self.childNumChild = childNumChild
        try:
            if not addrBase is None and not addrStep is None:
                self.d.put('addrbase="0x%x",' % long(addrBase))
                self.d.put('addrstep="0x%x",' % long(addrStep))
                self.printsAddress = False
        except:
            warn("ADDRBASE: %s" % addrBase)
        #warn("CHILDREN: %s %s %s" % (numChild, childType, childNumChild))

    def __enter__(self):
        self.savedChildType = self.d.currentChildType
        self.savedChildNumChild = self.d.currentChildNumChild
        self.savedNumChild = self.d.currentNumChild
        self.savedMaxNumChild = self.d.currentMaxNumChild
        self.savedPrintsAddress = self.d.currentPrintsAddress
        self.d.currentChildType = self.childType
        self.d.currentChildNumChild = self.childNumChild
        self.d.currentNumChild = self.numChild
        self.d.currentMaxNumChild = self.maxNumChild
        self.d.currentPrintsAddress = self.printsAddress
        self.d.put("children=[")

    def __exit__(self, exType, exValue, exTraceBack):
        if not exType is None:
            if self.d.passExceptions:
                showException("CHILDREN", exType, exValue, exTraceBack)
            self.d.putNumChild(0)
            self.d.putValue("<not accessible>")
        if not self.d.currentMaxNumChild is None:
            if self.d.currentMaxNumChild < self.d.currentNumChild:
                self.d.put('{name="<incomplete>",value="",type="",numchild="0"},')
        self.d.currentChildType = self.savedChildType
        self.d.currentChildNumChild = self.savedChildNumChild
        self.d.currentNumChild = self.savedNumChild
        self.d.currentMaxNumChild = self.savedMaxNumChild
        self.d.currentPrintsAddress = self.savedPrintsAddress
        self.d.put('],')
        return True


def value(expr):
    value = parseAndEvaluate(expr)
    try:
        return int(value)
    except:
        return str(value)

def isSimpleType(typeobj):
    code = typeobj.code
    return code == BoolCode \
        or code == CharCode \
        or code == IntCode \
        or code == FloatCode \
        or code == EnumCode \
        or code == SimpleValueCode

def simpleEncoding(typeobj):
    code = typeobj.code
    if code == BoolCode or code == CharCode:
        return Hex2EncodedInt1
    if code == IntCode:
        if str(typeobj).find("unsigned") >= 0:
            if typeobj.sizeof == 1:
                return Hex2EncodedUInt1
            if typeobj.sizeof == 2:
                return Hex2EncodedUInt2
            if typeobj.sizeof == 4:
                return Hex2EncodedUInt4
            if typeobj.sizeof == 8:
                return Hex2EncodedUInt8
        else:
            if typeobj.sizeof == 1:
                return Hex2EncodedInt1
            if typeobj.sizeof == 2:
                return Hex2EncodedInt2
            if typeobj.sizeof == 4:
                return Hex2EncodedInt4
            if typeobj.sizeof == 8:
                return Hex2EncodedInt8
    if code == FloatCode:
        if typeobj.sizeof == 4:
            return Hex2EncodedFloat4
        if typeobj.sizeof == 8:
            return Hex2EncodedFloat8
    return None

def check(exp):
    if not exp:
        raise RuntimeError("Check failed")

#def couldBePointer(p, align):
#    type = lookupType("unsigned int")
#    ptr = gdb.Value(p).cast(type)
#    d = int(str(ptr))
#    warn("CHECKING : %s %d " % (p, ((d & 3) == 0 and (d > 1000 or d == 0))))
#    return (d & (align - 1)) and (d > 1000 or d == 0)


def checkAccess(p, align = 1):
    return p.dereference()

def checkContents(p, expected, align = 1):
    if int(p.dereference()) != expected:
        raise RuntimeError("Contents check failed")

def checkPointer(p, align = 1):
    if not isNull(p):
        p.dereference()

def pointerValue(p):
    return long(p)

def isNull(p):
    # The following can cause evaluation to abort with "UnicodeEncodeError"
    # for invalid char *, as their "contents" is being examined
    #s = str(p)
    #return s == "0x0" or s.startswith("0x0 ")
    #try:
    #    # Can fail with: "RuntimeError: Cannot access memory at address 0x5"
    #    return p.cast(lookupType("void").pointer()) == 0
    #except:
    #    return False
    try:
        # Can fail with: "RuntimeError: Cannot access memory at address 0x5"
        return long(p) == 0
    except:
        return False

Value = gdb.Value

def stripClassTag(typeName):
    if typeName.startswith("class "):
        return typeName[6:]
    if typeName.startswith("struct "):
        return typeName[7:]
    if typeName.startswith("const "):
        return typeName[6:]
    if typeName.startswith("volatile "):
        return typeName[9:]
    return typeName

def checkPointerRange(p, n):
    for i in xrange(n):
        checkPointer(p)
        ++p

def call2(value, func, args):
    # args is a tuple.
    arg = ""
    for i in range(len(args)):
        if i:
            arg += ','
        a = args[i]
        if (':' in a) and not ("'" in a):
            arg = "'%s'" % a
        else:
            arg += a

    #warn("CALL: %s -> %s(%s)" % (value, func, arg))
    type = stripClassTag(str(value.type))
    if type.find(":") >= 0:
        type = "'" + type + "'"
    # 'class' is needed, see http://sourceware.org/bugzilla/show_bug.cgi?id=11912
    exp = "((class %s*)%s)->%s(%s)" % (type, value.address, func, arg)
    #warn("CALL: %s" % exp)
    result = None
    try:
        result = parseAndEvaluate(exp)
    except:
        pass
    #warn("  -> %s" % result)
    return result

def call(value, func, *args):
    return call2(value, func, args)

def makeValue(type, init):
    type = "::" + stripClassTag(str(type));
    # Avoid malloc symbol clash with QVector.
    gdb.execute("set $d = (%s*)calloc(sizeof(%s), 1)" % (type, type))
    gdb.execute("set *$d = {%s}" % init)
    value = parseAndEvaluate("$d").dereference()
    #warn("  TYPE: %s" % value.type)
    #warn("  ADDR: %s" % value.address)
    #warn("  VALUE: %s" % value)
    return value

def makeStdString(init):
    # Works only for small allocators, but they are usually empty.
    gdb.execute("set $d=(std::string*)calloc(sizeof(std::string), 2)");
    gdb.execute("call($d->basic_string(\"" + init +
        "\",*(std::allocator<char>*)(1+$d)))")
    value = parseAndEvaluate("$d").dereference()
    #warn("  TYPE: %s" % value.type)
    #warn("  ADDR: %s" % value.address)
    #warn("  VALUE: %s" % value)
    return value


def makeExpression(value):
    type = "::" + stripClassTag(str(value.type))
    #warn("  TYPE: %s" % type)
    #exp = "(*(%s*)(&%s))" % (type, value.address)
    exp = "(*(%s*)(%s))" % (type, value.address)
    #warn("  EXP: %s" % exp)
    return exp

qqNs = None

def qtNamespace():
    # FIXME: This only works when call from inside a Qt function frame.
    global qqNs
    if not qqNs is None:
        return qqNs
    try:
        str = gdb.execute("ptype QString::Null", to_string=True)
        # The result looks like:
        # "type = const struct myns::QString::Null {"
        # "    <no data fields>"
        # "}"
        pos1 = str.find("struct") + 7
        pos2 = str.find("QString::Null")
        if pos1 > -1 and pos2 > -1:
            qqNs = str[pos1:pos2]
            return qqNs
        return ""
    except:
        return ""

def findFirstZero(p, maximum):
    for i in xrange(maximum):
        if p.dereference() == 0:
            return i
        p = p + 1
    return maximum + 1

def encodeCArray(p, innerType, suffix):
    t = lookupType(innerType)
    p = p.cast(t.pointer())
    limit = findFirstZero(p, qqStringCutOff)
    s = readRawMemory(p, limit * t.sizeof)
    if limit > qqStringCutOff:
        s += suffix
    return s

def encodeCharArray(p):
    return encodeCArray(p, "unsigned char", "2e2e2e")

def encodeChar2Array(p):
    return encodeCArray(p, "unsigned short", "2e002e002e00")

def encodeChar4Array(p):
    return encodeCArray(p, "unsigned int", "2e0000002e0000002e000000")

def computeLimit(size, limit):
    if limit is None:
        return size
    if limit == 0:
        return min(size, qqStringCutOff)
    return min(size, limit)

def stripTypedefs(type):
    type = type.unqualified()
    while type.code == TypedefCode:
        type = type.strip_typedefs().unqualified()
    return type


#######################################################################
#
# LocalItem
#
#######################################################################

# Contains iname, name, and value.
class LocalItem:
    pass

#######################################################################
#
# SetupCommand
#
#######################################################################

# This keeps canonical forms of the typenames, without array indices etc.
qqStripForFormat = {}

def stripForFormat(typeName):
    global qqStripForFormat
    if typeName in qqStripForFormat:
        return qqStripForFormat[typeName]
    stripped = ""
    inArray = 0
    for c in stripClassTag(typeName):
        if c == '<':
            break
        if c == ' ':
            continue
        if c == '[':
            inArray += 1
        elif c == ']':
            inArray -= 1
        if inArray and ord(c) >= 48 and ord(c) <= 57:
            continue
        stripped +=  c
    qqStripForFormat[typeName] = stripped
    return stripped



#######################################################################
#
# Edit Command
#
#######################################################################

def bbedit(args):
    global qqEditable
    (type, expr, value) = args.split(",")
    type = base64.b16decode(type, True)
    ns = qtNamespace()
    if type.startswith(ns):
        type = type[len(ns):]
    type = type.replace("::", "__")
    pos = type.find('<')
    if pos != -1:
        type = type[0:pos]
    expr = base64.b16decode(expr, True)
    value = base64.b16decode(value, True)
    #warn("EDIT: %s %s %s %s: " % (pos, type, expr, value))
    if qqEditable.has_key(type):
        qqEditable[type](expr, value)
    else:
        gdb.execute("set (%s)=%s" % (expr, value))

registerCommand("bbedit", bbedit)


#######################################################################
#
# Frame Command
#
#######################################################################

typesToReport = {}

def bb(args):
    global typesToReport
    output = Dumper(args).output
    output.append('],typeinfo=[')
    for name, type in typesToReport.iteritems():
        # Happens e.g. for '(anonymous namespace)::InsertDefOperation'
        if not type is None:
            output.append('{name="%s",size="%s"}'
                % (base64.b64encode(name), type.sizeof))
    output.append(']')
    typesToReport = {}
    return "".join(output)


def p1(args):
    import cProfile
    cProfile.run('bb("%s")' % args, "/tmp/bbprof")
    import pstats
    pstats.Stats('/tmp/bbprof').sort_stats('time').print_stats()
    return ""


def p2(args):
    import timeit
    return timeit.repeat('bb("%s")' % args,
        'from __main__ import bb', number=10)

registerCommand("bb", bb)
registerCommand("p1", p1)
registerCommand("p2", p2)


#######################################################################
#
# The Dumper Class
#
#######################################################################


class Dumper:
    def defaultInit(self):
        self.output = []
        self.currentIName = ""
        self.currentPrintsAddress = True
        self.currentChildType = ""
        self.currentChildNumChild = -1
        self.currentMaxNumChild = -1
        self.currentNumChild = -1
        self.currentValue = None
        self.currentValuePriority = -100
        self.currentValueEncoding = None
        self.currentType = None
        self.currentTypePriority = -100
        self.currentAddress = None
        self.typeformats = {}
        self.formats = {}
        self.useDynamicType = True
        self.expandedINames = {}
        self.childEventAddress = None

    def __init__(self, args):
        self.defaultInit()

        watchers = ""
        resultVarName = ""
        options = []
        varList = []

        self.output.append('data=[')
        for arg in args.split(' '):
            pos = arg.find(":") + 1
            if arg.startswith("options:"):
                options = arg[pos:].split(",")
            elif arg.startswith("vars:"):
                if len(arg[pos:]) > 0:
                    varList = arg[pos:].split(",")
            elif arg.startswith("resultvarname:"):
                resultVarName = arg[pos:]
            elif arg.startswith("expanded:"):
                self.expandedINames = set(arg[pos:].split(","))
            elif arg.startswith("typeformats:"):
                for f in arg[pos:].split(","):
                    pos = f.find("=")
                    if pos != -1:
                        type = base64.b16decode(f[0:pos], True)
                        self.typeformats[type] = int(f[pos+1:])
            elif arg.startswith("formats:"):
                for f in arg[pos:].split(","):
                    pos = f.find("=")
                    if pos != -1:
                        self.formats[f[0:pos]] = int(f[pos+1:])
            elif arg.startswith("watchers:"):
                watchers = base64.b16decode(arg[pos:], True)

        self.useDynamicType = "dyntype" in options
        self.useFancy = "fancy" in options
        self.passExceptions = "pe" in options
        #self.passExceptions = True
        self.autoDerefPointers = "autoderef" in options
        self.partialUpdate = "partial" in options
        self.tooltipOnly = "tooltiponly" in options
        self.noLocals = "nolocals" in options
        self.ns = qtNamespace()

        #warn("NAMESPACE: '%s'" % self.ns)
        #warn("VARIABLES: %s" % varList)
        #warn("EXPANDED INAMES: %s" % self.expandedINames)
        #warn("WATCHERS: %s" % watchers)
        #warn("PARTIAL: %s" % self.partialUpdate)
        #warn("NO LOCALS: %s" % self.noLocals)
        module = sys.modules[__name__]

        #
        # Locals
        #
        locals = []
        fullUpdateNeeded = True
        if self.partialUpdate and len(varList) == 1 and not self.tooltipOnly:
            #warn("PARTIAL: %s" % varList)
            parts = varList[0].split('.')
            #warn("PARTIAL PARTS: %s" % parts)
            name = parts[1]
            #warn("PARTIAL VAR: %s" % name)
            #fullUpdateNeeded = False
            try:
                frame = gdb.selected_frame()
                item = LocalItem()
                item.iname = "local." + name
                item.name = name
                item.value = frame.read_var(name)
                locals = [item]
                #warn("PARTIAL LOCALS: %s" % locals)
                fullUpdateNeeded = False
            except:
                pass
            varList = []

        if fullUpdateNeeded and not self.tooltipOnly and not self.noLocals:
            locals = listOfLocals(varList)

        # Take care of the return value of the last function call.
        if len(resultVarName) > 0:
            try:
                item = LocalItem()
                item.name = resultVarName
                item.iname = "return." + resultVarName
                item.value = parseAndEvaluate(resultVarName)
                locals.append(item)
            except:
                # Don't bother. It's only supplementary information anyway.
                pass

        for item in locals:
            value = downcast(item.value) if self.useDynamicType else item.value
            with OutputSafer(self):
                self.anonNumber = -1

                type = value.type.unqualified()
                typeName = str(type)

                # Special handling for char** argv.
                if type.code == PointerCode \
                        and item.iname == "local.argv" \
                        and typeName == "char **":
                    n = 0
                    p = value
                    # p is 0 for "optimized out" cases. Or contains rubbish.
                    try:
                        if not isNull(p):
                            while not isNull(p.dereference()) and n <= 100:
                                p += 1
                                n += 1
                    except:
                        pass

                    with TopLevelItem(self, item.iname):
                        self.put('iname="local.argv",name="argv",')
                        self.putItemCount(n, 100)
                        self.putType(typeName)
                        self.putNumChild(n)
                        if self.currentIName in self.expandedINames:
                            p = value
                            with Children(self, n):
                                for i in xrange(n):
                                    self.putSubItem(i, p.dereference())
                                    p += 1
                    continue

                else:
                    # A "normal" local variable or parameter.
                    with TopLevelItem(self, item.iname):
                        self.put('iname="%s",' % item.iname)
                        self.put('name="%s",' % item.name)
                        self.putItem(value)

        #
        # Watchers
        #
        with OutputSafer(self):
            if len(watchers) > 0:
                self.put(",")
                for watcher in watchers.split("##"):
                    (exp, iname) = watcher.split("#")
                    self.handleWatch(exp, iname)

        #print('data=[' + locals + sep + watchers + ']\n')


    def templateArgument(self, typeobj, position):
        return templateArgument(typeobj, position)

    def numericTemplateArgument(self, typeobj, position):
        return numericTemplateArgument(typeobj, position)

    def lookupType(self, typeName):
        return lookupType(typeName)

    def handleWatch(self, exp, iname):
        exp = str(exp)
        escapedExp = base64.b64encode(exp);
        #warn("HANDLING WATCH %s, INAME: '%s'" % (exp, iname))
        if exp.startswith("[") and exp.endswith("]"):
            #warn("EVAL: EXP: %s" % exp)
            with TopLevelItem(self, iname):
                self.put('iname="%s",' % iname)
                self.put('wname="%s",' % escapedExp)
                try:
                    list = eval(exp)
                    self.putValue("")
                    self.putNoType()
                    self.putNumChild(len(list))
                    # This is a list of expressions to evaluate
                    with Children(self, len(list)):
                        itemNumber = 0
                        for item in list:
                            self.handleWatch(item, "%s.%d" % (iname, itemNumber))
                            itemNumber += 1
                except RuntimeError, error:
                    warn("EVAL: ERROR CAUGHT %s" % error)
                    self.putValue("<syntax error>")
                    self.putNoType()
                    self.putNumChild(0)
                    self.put("children=[],")
            return

        with TopLevelItem(self, iname):
            self.put('iname="%s",' % iname)
            self.put('wname="%s",' % escapedExp)
            if len(exp) == 0: # The <Edit> case
                self.putValue(" ")
                self.putNoType()
                self.putNumChild(0)
            else:
                try:
                    value = parseAndEvaluate(exp)
                    self.putItem(value)
                except RuntimeError:
                    self.currentType = " "
                    self.currentValue = "<no such value>"
                    self.currentChildNumChild = -1
                    self.currentNumChild = 0
                    self.putNumChild(0)

    def intType(self):
        return self.lookupType('int')

    def charType(self):
        return self.lookupType('char')

    def sizetType(self):
        return self.lookupType('size_t')

    def charPtrType(self):
        return self.lookupType('char*')

    def intPtrType(self):
        return self.lookupType('int*')

    def voidPtrType(self):
        return self.lookupType('void*')

    def addressOf(self, value):
        return long(value.address)

    def createPointerValue(self, address, pointeeType):
        return gdb.Value(address).cast(pointeeType.pointer())

    def intSize(self):
        return 4

    def ptrSize(self):
        return self.lookupType('void*').sizeof

    def is32bit(self):
        return self.lookupType('void*').sizeof == 4

    def createValue(self, address, referencedType):
        return gdb.Value(address).cast(referencedType.pointer()).dereference()

    def dereference(self, addr):
        return long(gdb.Value(addr).cast(self.voidPtrType().pointer()).dereference())

    def extractInt(self, addr):
        return long(gdb.Value(addr).cast(self.intPtrType()).dereference())

    # Do not use value.address here as this might not have one,
    # i.e. be the result of an inferior call
    def dereferenceValue(self, value):
        return value.cast(self.voidPtrType())

    def isQObject(self, value):
        try:
        #if True:
            vtable = self.dereference(long(value.address)) # + ptrSize
            metaObjectEntry = self.dereference(vtable) # It's the first entry.
            #warn("MO: 0x%x " % metaObjectEntry)
            s = gdb.execute("info symbol 0x%x" % metaObjectEntry, to_string=True)
            #warn("S: %s " % s)
            #return s.find("::metaObject() const") > 0
            return s.find("::metaObject() const") > 0 or s.find("10metaObjectEv") > 0
            #return str(metaObjectEntry).find("::metaObject() const") > 0
        except:
            return False

    def isQObject_B(self, value):
        # Alternative: Check for specific values, like targeting the
        # 'childEvent' member which is typically not overwritten, slot 8.
        # ~"Symbol \"Myns::QObject::childEvent(Myns::QChildEvent*)\" is a
        #  function at address 0xb70f691a.\n"
        if self.childEventAddress == None:
            try:
                loc = gdb.execute("info address ::QObject::childEvent", to_string=True)
                self.childEventAddress = long(loc[loc.rfind(' '):-2], 16)
            except:
                self.childEventAddress = 0

        try:
            vtable = self.dereference(long(value.address))
            return self.childEventAddress == self.dereference(vtable + 8 * self.ptrSize())
        except:
            return False

    def put(self, value):
        self.output.append(value)

    def putField(self, name, value):
        self.put('%s="%s",' % (name, value))

    def childRange(self):
        if self.currentMaxNumChild is None:
            return xrange(0, self.currentNumChild)
        return xrange(min(self.currentMaxNumChild, self.currentNumChild))

    def qtVersion(self):
        global qqVersion
        if not qqVersion is None:
            return qqVersion
        try:
            # This will fail on Qt 5
            gdb.execute("ptype QString::shared_empty", to_string=True)
            qqVersion = 0x040800
        except:
            qqVersion = 0x050000
        return qqVersion

    # Convenience function.
    def putItemCount(self, count, maximum = 1000000000):
        # This needs to override the default value, so don't use 'put' directly.
        if count > maximum:
            self.putValue('<>%s items>' % maximum)
        else:
            self.putValue('<%s items>' % count)

    def putType(self, type, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentTypePriority:
            self.currentType = str(type)
            self.currentTypePriority = priority

    def putNoType(self):
        # FIXME: replace with something that does not need special handling
        # in SubItem.__exit__().
        self.putBetterType(" ")

    def putInaccessible(self):
        #self.putBetterType(" ")
        self.putNumChild(0)
        self.currentValue = None

    def putBetterType(self, type):
        self.currentType = str(type)
        self.currentTypePriority = self.currentTypePriority + 1

    def putAddress(self, addr):
        if self.currentPrintsAddress:
            try:
                # addr can be "None", long(None) fails.
                #self.put('addr="0x%x",' % long(addr))
                self.currentAddress = 'addr="0x%x",' % long(addr)
            except:
                pass

    def putNumChild(self, numchild):
        #warn("NUM CHILD: '%s' '%s'" % (numchild, self.currentChildNumChild))
        if numchild != self.currentChildNumChild:
            self.put('numchild="%s",' % numchild)

    def putEmptyValue(self, priority = -10):
        if priority >= self.currentValuePriority:
            self.currentValue = ""
            self.currentValuePriority = priority
            self.currentValueEncoding = None

    def putValue(self, value, encoding = None, priority = 0):
        # Higher priority values override lower ones.
        if priority >= self.currentValuePriority:
            self.currentValue = value
            self.currentValuePriority = priority
            self.currentValueEncoding = encoding

    def putPointerValue(self, value):
        # Use a lower priority
        if value is None:
            self.putEmptyValue(-1)
        else:
            self.putValue("0x%x" % value.cast(
                lookupType("unsigned long")), None, -1)

    def putDisplay(self, format, value = None, cmd = None):
        self.put('editformat="%s",' % format)
        if cmd is None:
            if not value is None:
                self.put('editvalue="%s",' % value)
        else:
            self.put('editvalue="%s|%s",' % (cmd, value))

    def computeLimit(self, size, limit):
        if limit is None:
            return size
        if limit == 0:
            return min(size, qqStringCutOff)
        return min(size, limit)

    def putName(self, name):
        self.put('name="%s",' % name)

    def isExpanded(self):
        #warn("IS EXPANDED: %s in %s: %s" % (self.currentIName,
        #    self.expandedINames, self.currentIName in self.expandedINames))
        return self.currentIName in self.expandedINames

    def isExpandedSubItem(self, component):
        iname = "%s.%s" % (self.currentIName, component)
        #warn("IS EXPANDED: %s in %s" % (iname, self.expandedINames))
        return iname in self.expandedINames

    def stripNamespaceFromType(self, typeName):
        type = stripClassTag(typeName)
        if len(self.ns) > 0 and type.startswith(self.ns):
            type = type[len(self.ns):]
        pos = type.find("<")
        # FIXME: make it recognize  foo<A>::bar<B>::iterator?
        while pos != -1:
            pos1 = type.rfind(">", pos)
            type = type[0:pos] + type[pos1+1:]
            pos = type.find("<")
        return type

    def isMovableType(self, type):
        if type.code == PointerCode:
            return True
        if isSimpleType(type):
            return True
        return self.stripNamespaceFromType(str(type)) in movableTypes

    def putIntItem(self, name, value):
        with SubItem(self, name):
            self.putValue(value)
            self.putType("int")
            self.putNumChild(0)

    def putBoolItem(self, name, value):
        with SubItem(self, name):
            self.putValue(value)
            self.putType("bool")
            self.putNumChild(0)

    def currentItemFormat(self):
        format = self.formats.get(self.currentIName)
        if format is None:
            format = self.typeformats.get(stripForFormat(str(self.currentType)))
        return format

    def putSubItem(self, component, value, tryDynamic=True):
        with SubItem(self, component):
            self.putItem(value, tryDynamic)

    def putNamedSubItem(self, component, value, name):
        with SubItem(self, component):
            self.putName(name)
            self.putItem(value)

    def tryPutArrayContents(self, typeobj, base, n):
        if not isSimpleType(typeobj):
            return False
        size = n * typeobj.sizeof;
        self.put('childtype="%s",' % typeobj)
        self.put('addrbase="0x%x",' % long(base))
        self.put('addrstep="0x%x",' % long(typeobj.sizeof))
        self.put('arrayencoding="%s",' % simpleEncoding(typeobj))
        self.put('arraydata="')
        self.put(self.readRawMemory(base, size))
        self.put('",')
        return True

    def isReferenceType(self, typeobj):
        return typeobj.code == gdb.TYPE_CODE_REF

    def isStructType(self, typeobj):
        return typeobj.code == gdb.TYPE_CODE_STRUCT

    def putPlotData(self, type, base, n, plotFormat):
        if self.isExpanded():
            self.putArrayData(type, base, n)
        if not hasPlot():
            return
        if not isSimpleType(type):
            #self.putValue(self.currentValue + " (not plottable)")
            self.putValue(self.currentValue)
            self.putField("plottable", "0")
            return
        global gnuplotPipe
        global gnuplotPid
        format = self.currentItemFormat()
        iname = self.currentIName
        #if False:
        if format != plotFormat:
            if iname in gnuplotPipe:
                os.kill(gnuplotPid[iname], 9)
                del gnuplotPid[iname]
                gnuplotPipe[iname].terminate()
                del gnuplotPipe[iname]
            return
        base = base.cast(type.pointer())
        if not iname in gnuplotPipe:
            gnuplotPipe[iname] = subprocess.Popen(["gnuplot"],
                    stdin=subprocess.PIPE)
            gnuplotPid[iname] = gnuplotPipe[iname].pid
        f = gnuplotPipe[iname].stdin;
        f.write("set term wxt noraise\n")
        f.write("set title 'Data fields'\n")
        f.write("set xlabel 'Index'\n")
        f.write("set ylabel 'Value'\n")
        f.write("set grid\n")
        f.write("set style data lines;\n")
        f.write("plot  '-' title '%s'\n" % iname)
        for i in range(1, n):
            f.write(" %s\n" % base.dereference())
            base += 1
        f.write("e\n")


    def putArrayData(self, type, base, n,
            childNumChild = None, maxNumChild = 10000):
        if not self.tryPutArrayContents(type, base, n):
            base = base.cast(type.pointer())
            with Children(self, n, type, childNumChild, maxNumChild,
                    base, type.sizeof):
                for i in self.childRange():
                    self.putSubItem(i, (base + i).dereference())


    def putCallItem(self, name, value, func, *args):
        result = call2(value, func, args)
        with SubItem(self, name):
            self.putItem(result)

    def putItem(self, value, tryDynamic=True):
        if value is None:
            # Happens for non-available watchers in gdb versions that
            # need to use gdb.execute instead of gdb.parse_and_eval
            self.putValue("<not available>")
            self.putType("<unknown>")
            self.putNumChild(0)
            return

        global qqDumpers, qqFormats

        type = value.type.unqualified()
        typeName = str(type)
        tryDynamic &= self.useDynamicType
        lookupType(typeName) # Fill type cache
        if tryDynamic:
            self.putAddress(value.address)

        # FIXME: Gui shows references stripped?
        #warn(" ")
        #warn("REAL INAME: %s " % self.currentIName)
        #warn("REAL TYPE: %s " % value.type)
        #warn("REAL CODE: %s " % value.type.code)
        #warn("REAL VALUE: %s " % value)

        if type.code == ReferenceCode:
            try:
                # Try to recognize null references explicitly.
                if long(value.address) == 0:
                    self.putValue("<null reference>")
                    self.putType(typeName)
                    self.putNumChild(0)
                    return
            except:
                pass

            if tryDynamic:
                try:
                    # Dynamic references are not supported by gdb, see
                    # http://sourceware.org/bugzilla/show_bug.cgi?id=14077.
                    # Find the dynamic type manually using referenced_type.
                    value = value.referenced_value()
                    value = value.cast(value.dynamic_type)
                    self.putItem(value)
                    self.putBetterType("%s &" % value.type)
                    return
                except:
                    pass

            try:
                # FIXME: This throws "RuntimeError: Attempt to dereference a
                # generic pointer." with MinGW's gcc 4.5 when it "identifies"
                # a "QWidget &" as "void &" and with optimized out code.
                self.putItem(value.cast(type.target().unqualified()))
                self.putBetterType(typeName)
                return
            except RuntimeError:
                self.putValue("<optimized out reference>")
                self.putType(typeName)
                self.putNumChild(0)
                return

        if type.code == IntCode or type.code == CharCode or type.code == SimpleValueCode:
            self.putType(typeName)
            if value.is_optimized_out:
                self.putValue("<optimized out>")
            else:
                self.putValue(value)
            self.putNumChild(0)
            return

        if type.code == FloatCode or type.code == BoolCode:
            self.putType(typeName)
            if value.is_optimized_out:
                self.putValue("<optimized out>")
            else:
                self.putValue(value)
            self.putNumChild(0)
            return

        if type.code == EnumCode:
            self.putType(typeName)
            if value.is_optimized_out:
                self.putValue("<optimized out>")
            else:
                self.putValue("%s (%d)" % (value, value))
            self.putNumChild(0)
            return

        if type.code == ComplexCode:
            self.putType(typeName)
            if value.is_optimized_out:
                self.putValue("<optimized out>")
            else:
                self.putValue("%s" % value)
            self.putNumChild(0)
            return

        if type.code == TypedefCode:
            if typeName in qqDumpers:
                self.putType(typeName)
                qqDumpers[typeName](self, value)
                return

            type = stripTypedefs(type)
            # The cast can destroy the address?
            #self.putAddress(value.address)
            # Workaround for http://sourceware.org/bugzilla/show_bug.cgi?id=13380
            if type.code == ArrayCode:
                value = parseAndEvaluate("{%s}%s" % (type, value.address))
            else:
                try:
                    value = value.cast(type)
                except:
                    self.putValue("<optimized out typedef>")
                    self.putType(typeName)
                    self.putNumChild(0)
                    return

            self.putItem(value)
            self.putBetterType(typeName)
            return

        if type.code == ArrayCode:
            qdump____c_style_array__(self, value)
            return

        if type.code == PointerCode:
            #warn("POINTER: %s" % value)

            # This could still be stored in a register and
            # potentially dereferencable.
            if value.is_optimized_out:
                self.putValue("<optimized out>")

            try:
                value.dereference()
            except:
                # Failure to dereference a pointer should at least
                # show the value of a pointer.
                self.putValue(cleanAddress(value))
                self.putType(typeName)
                self.putNumChild(0)
                return

            if isNull(value):
                #warn("NULL POINTER")
                self.putType(typeName)
                self.putValue("0x0")
                self.putNumChild(0)
                return

            innerType = type.target()
            innerTypeName = str(innerType.unqualified())
            format = self.formats.get(self.currentIName)
            if format is None:
                format = self.typeformats.get(stripForFormat(str(type)))

            if innerType.code == VoidCode:
                #warn("VOID POINTER: %s" % format)
                self.putType(typeName)
                self.putValue(str(value))
                self.putNumChild(0)
                return

            if format == None and innerTypeName == "char":
                # Use Latin1 as default for char *.
                self.putType(typeName)
                self.putValue(encodeCharArray(value), Hex2EncodedLatin1)
                self.putNumChild(0)
                return

            if format == 0:
                # Explicitly requested bald pointer.
                self.putType(typeName)
                self.putPointerValue(value)
                self.putNumChild(1)
                if self.currentIName in self.expandedINames:
                    with Children(self):
                        with SubItem(self, '*'):
                            self.putItem(value.dereference())
                return

            if format == 1:
                # Explicitly requested Latin1 formatting.
                self.putType(typeName)
                self.putValue(encodeCharArray(value), Hex2EncodedLatin1)
                self.putNumChild(0)
                return

            if format == 2:
                # Explicitly requested UTF-8 formatting.
                self.putType(typeName)
                self.putValue(encodeCharArray(value), Hex2EncodedUtf8)
                self.putNumChild(0)
                return

            if format == 3:
                # Explicitly requested local 8 bit formatting.
                self.putType(typeName)
                self.putValue(encodeCharArray(value), Hex2EncodedLocal8Bit)
                self.putNumChild(0)
                return

            if format == 4:
                # Explicitly requested UTF-16 formatting.
                self.putType(typeName)
                self.putValue(encodeChar2Array(value), Hex4EncodedLittleEndian)
                self.putNumChild(0)
                return

            if format == 5:
                # Explicitly requested UCS-4 formatting.
                self.putType(typeName)
                self.putValue(encodeChar4Array(value), Hex8EncodedLittleEndian)
                self.putNumChild(0)
                return

            if format == 6:
                # Explicitly requested formatting as array of 10 items.
                self.putType(typeName)
                self.putItemCount(10)
                self.putNumChild(10)
                self.putArrayData(innerType, value, 10)
                return

            if format == 7:
                # Explicitly requested formatting as array of 1000 items.
                self.putType(typeName)
                self.putItemCount(1000)
                self.putNumChild(1000)
                self.putArrayData(innerType, value, 1000)
                return

            if innerType.code == MethodCode or innerType.code == FunctionCode:
                # A function pointer with format None.
                self.putValue(str(value))
                self.putType(typeName)
                self.putNumChild(0)
                return

            #warn("AUTODEREF: %s" % self.autoDerefPointers)
            #warn("INAME: %s" % self.currentIName)
            if self.autoDerefPointers or self.currentIName.endswith('.this'):
                ## Generic pointer type with format None
                #warn("GENERIC AUTODEREF POINTER: %s AT %s TO %s"
                #    % (type, value.address, innerTypeName))
                # Never dereference char types.
                if innerTypeName != "char" \
                        and innerTypeName != "signed char" \
                        and innerTypeName != "unsigned char"  \
                        and innerTypeName != "wchar_t":
                    self.putType(innerType)
                    savedCurrentChildType = self.currentChildType
                    self.currentChildType = stripClassTag(innerTypeName)
                    self.putItem(value.dereference())
                    self.currentChildType = savedCurrentChildType
                    #self.putPointerValue(value)
                    self.put('origaddr="%s",' % value.address)
                    return

            # Fall back to plain pointer printing.
            #warn("GENERIC PLAIN POINTER: %s" % value.type)
            #warn("ADDR PLAIN POINTER: %s" % value.address)
            self.putType(typeName)
            self.putField("aaa", "1")
            #self.put('addr="0x%x",' % long(value.address))
            #self.putAddress(value.address)
            self.putField("bbb", "1")
            #self.putPointerValue(value)
            self.putValue("0x%x" % value.cast(lookupType("unsigned long")))
            self.putField("ccc", "1")
            self.putNumChild(1)
            if self.currentIName in self.expandedINames:
                with Children(self):
                    with SubItem(self, "*"):
                        self.putItem(value.dereference())
            return

        if type.code == MethodPointerCode \
                or type.code == MethodCode \
                or type.code == FunctionCode \
                or type.code == MemberPointerCode:
            self.putType(typeName)
            self.putValue(value)
            self.putNumChild(0)
            return

        if typeName.startswith("<anon"):
            # Anonymous union. We need a dummy name to distinguish
            # multiple anonymous unions in the struct.
            self.putType(type)
            self.putValue("{...}")
            self.anonNumber += 1
            with Children(self, 1):
                self.listAnonymous(value, "#%d" % self.anonNumber, type)
            return

        if type.code != StructCode and type.code != UnionCode:
            warn("WRONG ASSUMPTION HERE: %s " % type.code)
            check(False)


        if tryDynamic:
            self.putItem(expensiveDowncast(value), False)
            return

        format = self.formats.get(self.currentIName)
        if format is None:
            format = self.typeformats.get(stripForFormat(typeName))

        if self.useFancy and (format is None or format >= 1):
            self.putType(typeName)

            nsStrippedType = self.stripNamespaceFromType(typeName)\
                .replace("::", "__")

            # The following block is only needed for D.
            if nsStrippedType.startswith("_A"):
                # DMD v2.058 encodes string[] as _Array_uns long long.
                # With spaces.
                if nsStrippedType.startswith("_Array_"):
                    qdump_Array(self, value)
                    return
                if nsStrippedType.startswith("_AArray_"):
                    qdump_AArray(self, value)
                    return

            #warn(" STRIPPED: %s" % nsStrippedType)
            #warn(" DUMPERS: %s" % qqDumpers)
            #warn(" DUMPERS: %s" % (nsStrippedType in qqDumpers))
            dumper = qqDumpers.get(nsStrippedType, None)
            if not dumper is None:
                if tryDynamic:
                    dumper(self, expensiveDowncast(value))
                else:
                    dumper(self, value)
                return

        # D arrays, gdc compiled.
        if typeName.endswith("[]"):
            n = value["length"]
            base = value["ptr"]
            self.putType(typeName)
            self.putItemCount(n)
            if self.isExpanded():
                self.putArrayData(base.type.target(), base, n)
            return

        #warn("GENERIC STRUCT: %s" % type)
        #warn("INAME: %s " % self.currentIName)
        #warn("INAMES: %s " % self.expandedINames)
        #warn("EXPANDED: %s " % (self.currentIName in self.expandedINames))
        if self.isQObject(value):
            self.putQObjectNameValue(value)  # Is this too expensive?
        self.putType(typeName)
        self.putEmptyValue()
        self.putNumChild(fieldCount(type))

        if self.currentIName in self.expandedINames:
            innerType = None
            with Children(self, 1, childType=innerType):
                self.putFields(value)

    def putPlainChildren(self, value):
        self.putEmptyValue(-99)
        self.putNumChild(1)
        if self.currentIName in self.expandedINames:
            with Children(self):
               self.putFields(value)

    def readRawMemory(self, base, size):
        return readRawMemory(base, size)

    def putFields(self, value, dumpBase = True):
            fields = extractFields(value)

            #warn("TYPE: %s" % type)
            #warn("FIELDS: %s" % fields)
            baseNumber = 0
            for field in fields:
                #warn("FIELD: %s" % field)
                #warn("  BITSIZE: %s" % field.bitsize)
                #warn("  ARTIFICIAL: %s" % field.artificial)

                if field.name is None:
                    type = stripTypedefs(value.type)
                    innerType = type.target()
                    p = value.cast(innerType.pointer())
                    for i in xrange(type.sizeof / innerType.sizeof):
                        with SubItem(self, i):
                            self.putItem(p.dereference())
                        p = p + 1
                    continue

                # Ignore vtable pointers for virtual inheritance.
                if field.name.startswith("_vptr."):
                    with SubItem(self, "[vptr]"):
                        # int (**)(void)
                        n = 100
                        self.putType(" ")
                        self.putValue(value[field.name])
                        self.putNumChild(n)
                        if self.isExpanded():
                            with Children(self):
                                p = value[field.name]
                                for i in xrange(n):
                                    if long(p.dereference()) != 0:
                                        with SubItem(self, i):
                                            self.putItem(p.dereference())
                                            self.putType(" ")
                                            p = p + 1
                    continue

                #warn("FIELD NAME: %s" % field.name)
                #warn("FIELD TYPE: %s" % field.type)
                if field.is_base_class:
                    # Field is base type. We cannot use field.name as part
                    # of the iname as it might contain spaces and other
                    # strange characters.
                    if dumpBase:
                        baseNumber += 1
                        with UnnamedSubItem(self, "@%d" % baseNumber):
                            self.put('iname="%s",' % self.currentIName)
                            self.put('name="[%s]",' % field.name)
                            self.putItem(value.cast(field.type), False)
                elif len(field.name) == 0:
                    # Anonymous union. We need a dummy name to distinguish
                    # multiple anonymous unions in the struct.
                    self.anonNumber += 1
                    self.listAnonymous(value, "#%d" % self.anonNumber,
                        field.type)
                else:
                    # Named field.
                    with SubItem(self, field.name):
                        #bitsize = getattr(field, "bitsize", None)
                        #if not bitsize is None:
                        #    self.put("bitsize=\"%s\"" % bitsize)
                        self.putItem(downcast(value[field.name]))


    def listAnonymous(self, value, name, type):
        for field in type.fields():
            #warn("FIELD NAME: %s" % field.name)
            if len(field.name) > 0:
                with SubItem(self, field.name):
                    self.putItem(value[field.name])
            else:
                # Further nested.
                self.anonNumber += 1
                name = "#%d" % self.anonNumber
                #iname = "%s.%s" % (selitem.iname, name)
                #child = SameItem(item.value, iname)
                with SubItem(self, name):
                    self.put('name="%s",' % name)
                    self.putEmptyValue()
                    fieldTypeName = str(field.type)
                    if fieldTypeName.endswith("<anonymous union>"):
                        self.putType("<anonymous union>")
                    elif fieldTypeName.endswith("<anonymous struct>"):
                        self.putType("<anonymous struct>")
                    else:
                        self.putType(fieldTypeName)
                    with Children(self, 1):
                        self.listAnonymous(value, name, field.type)

#######################################################################
#
# ThreadNames Command
#
#######################################################################

def threadname(arg):
    try:
        e = gdb.selected_frame()
    except:
        return
    d = Dumper("")
    out = ""
    maximalStackDepth = int(arg)
    ns = qtNamespace()
    while True:
        maximalStackDepth -= 1
        if maximalStackDepth < 0:
            break
        e = e.older()
        if e == None or e.name() == None:
            break
        if e.name() == ns + "QThreadPrivate::start" \
                or e.name() == "_ZN14QThreadPrivate5startEPv@4":
            try:
                thrptr = e.read_var("thr").dereference()
                obtype = lookupType(ns + "QObjectPrivate").pointer()
                d_ptr = thrptr["d_ptr"]["d"].cast(obtype).dereference()
                try:
                    objectName = d_ptr["objectName"]
                except: # Qt 5
                    p = d_ptr["extraData"]
                    if not isNull(p):
                        objectName = p.dereference()["objectName"]
                if not objectName is None:
                    data, size, alloc = d.stringData(objectName)
                    if size > 0:
                         s = d.readRawMemory(data, 2 * size)

                thread = gdb.selected_thread()
                inner = '{valueencoded="';
                inner += str(Hex4EncodedLittleEndianWithoutQuotes)+'",id="'
                inner += str(thread.num) + '",value="'
                inner += s
                #inner += d.encodeString(objectName)
                inner += '"},'

                out += inner
            except:
                pass
    return out


def threadnames(arg):
    out = '['
    oldthread = gdb.selected_thread()
    try:
        inferior = selectedInferior()
        for thread in inferior.threads():
            thread.switch()
            out += threadname(arg)
    except:
        pass
    oldthread.switch()
    return out + ']'

registerCommand("threadnames", threadnames)


#######################################################################
#
# Mixed C++/Qml debugging
#
#######################################################################

def qmlb(args):
    # executeCommand(command, to_string=True).split("\n")
    warm("RUNNING: break -f QScript::FunctionWrapper::proxyCall")
    output = catchCliOutput("rbreak -f QScript::FunctionWrapper::proxyCall")
    warn("OUTPUT: %s " % output)
    bp = output[0]
    warn("BP: %s " % bp)
    # BP: ['Breakpoint 3 at 0xf166e7: file .../qscriptfunction.cpp, line 75.\\n'] \n"
    pos = bp.find(' ') + 1
    warn("POS: %s " % pos)
    nr = bp[bp.find(' ') + 1 : bp.find(' at ')]
    warn("NR: %s " % nr)
    return bp

registerCommand("qmlb", qmlb)

currentDir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
execfile(os.path.join(currentDir, "qttypes.py"))

bbsetup()
