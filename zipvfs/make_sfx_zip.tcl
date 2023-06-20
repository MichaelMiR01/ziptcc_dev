#!/usr/bin/tclsh
# [make_sfx_zip /zipfile/ /outfile/ /sfxstub/]
# Adds an arbitrary 'sfx' to a zip file, and adjusts the central directory
# and file items to compensate for this extra data.

proc make_sfx_zip { zipfile outfile sfx_stubfile} {
    
    set in [open $zipfile r]
    fconfigure $in -translation binary -encoding binary
    
    set out [open $outfile w+]
    fconfigure $out -translation binary -encoding binary
    
    if {[file exists $sfx_stubfile]} {
        set sfx_stubchan [open $sfx_stubfile r]
        fconfigure $sfx_stubchan -translation binary -encoding binary
        fcopy $sfx_stubchan $out
        close $sfx_stubchan
    } else {
        error "Stubfile $sfx_stubfile not found"
    }
    
    set offset [tell $out]
    
    lappend report "sfx stub size: $offset [file size $sfx_stubfile]"

    fcopy $in $out
    
    set size [tell $out]
    
    # Now seek in $out to find the end of directory signature:
    # The structure itself is 24 bytes long, followed by a maximum of 64Kbytes text
    
    if { $size < 65559 } {
        set seek 0
    } else {
        set seek [expr { $size - 65559 } ]
    }
    #flush $out
    seek $out $seek 
    puts "seek $seek [tell $out]"
    
    set data [read $out]
    set start_of_end [string last "\x50\x4b\x05\x06" $data]
    
    set start_of_end [expr {$start_of_end + $seek}]
    lappend report "SEO: $start_of_end ([expr {$start_of_end-$size}]) [string length $data]"
    
    seek $out $start_of_end
    set end_of_ctrl_dir [read $out]
    
    binary scan $end_of_ctrl_dir issssiis eocd(signature) eocd(disknbr) eocd(ctrldirdisk) \
        eocd(numondisk) eocd(totalnum) eocd(dirsize) eocd(diroffset) eocd(comment_len)
    
    lappend report "End of central directory: [array get eocd]"
    
    seek $out [expr {$start_of_end+16}]
    
    #adjust offset of start of central directory by the length of our sfx stub
    puts -nonewline $out [binary format i [expr {$eocd(diroffset)+$offset}]]
    flush $out
    
    seek $out $start_of_end
    set end_of_ctrl_dir [read $out]
    binary scan $end_of_ctrl_dir issssiis eocd(signature) eocd(disknbr) eocd(ctrldirdisk) \
        eocd(numondisk) eocd(totalnum) eocd(dirsize) eocd(diroffset) eocd(comment_len)
    
    lappend report "New dir offset: $eocd(diroffset)"
    lappend report "Adjusting $eocd(totalnum) zip file items."
    
    seek $out $eocd(diroffset)
    for {set i 0} {$i <$eocd(totalnum)} {incr i} {
        set current_file [tell $out]
        set fileheader [read $out 46]
        binary scan $fileheader is2sss2ii2s3ssii x(sig) x(version) x(flags) x(method) \
            x(date) x(crc32) x(sizes) x(lengths) x(diskno) x(iattr) x(eattr) x(offset)
        
        if { $x(sig) != 33639248 } {
            error "Bad file header signature at item $i: $x(sig)" 
        }
        
        foreach size $x(lengths) var {filename extrafield comment} {
            if { $size > 0 } {
                set x($var) [read $out $size] 
            } else {
                set x($var) ""
            }
        }
        set next_file [tell $out]
        #lappend report "file $i: $x(offset) $x(sizes) $x(filename)"
        
        seek $out [expr {$current_file+42}]
        puts -nonewline $out [binary format i [expr {$x(offset)+$offset}]]
    
        #verify:
        flush $out
        seek $out $current_file
        set fileheader [read $out 46]
        #lappend report "old $x(offset) + $offset"    
        binary scan $fileheader is2sss2ii2s3ssii x(sig) x(version) x(flags) x(method) \
            x(date) x(crc32) x(sizes) x(lengths) x(diskno) x(iattr) x(eattr) x(offset)
        #lappend report "new $x(offset)"    
    
        seek $out $next_file        
    }
    close $in
    close $out
    puts [join $report \n]
}

if { $::argc > 2 } {
    make_sfx_zip {*}$::argv
}
#./make_sfx_zip.tcl includes.zip ntcc _ztcc