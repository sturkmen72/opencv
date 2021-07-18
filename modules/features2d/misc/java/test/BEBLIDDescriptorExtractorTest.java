package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;
import org.opencv.test.OpenCVTestRunner;
import org.opencv.xfeatures2d.BEBLID;

public class BEBLIDDescriptorExtractorTest extends OpenCVTestCase {

    BEBLID extractor;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        extractor = BEBLID.create((float) 6.75); // default (6.75, 100)
    }

    public void testCreate() {
        assertNotNull(extractor);
    }

    public void testDetectListOfMatListOfListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectListOfMatListOfListOfKeyPointListOfMat() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPointMat() {
        fail("Not yet implemented");
    }

    public void testEmpty() {
        fail("Not yet implemented");
    }

    public void testRead() {
        String filename = OpenCVTestRunner.getTempFileName("xml");
        writeFile(filename, "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.BEBLID</name>\n<scale_factor>1.</scale_factor>\n<n_bits>101</n_bits>\n</opencv_storage>\n");

        extractor.read(filename);

        assertEquals((float) 1.0, extractor.getScaleFactor());
        assertEquals(101, extractor.getNbits());
    }

    public void testReadYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");
        writeFile(filename, "%YAML:1.0\n---\nname: \"Feature2D.BEBLID\"\nscale_factor: 1.\nn_bits: 101\n");

        extractor.read(filename);

        assertEquals((float) 1.0, extractor.getScaleFactor());
        assertEquals(101, extractor.getNbits());
    }

    public void testWrite() {
        String filename = OpenCVTestRunner.getTempFileName("xml");

        extractor.write(filename);

        String truth = "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.BEBLID</name>\n<scale_factor>6.7500000000000000e+00</scale_factor>\n<n_bits>100</n_bits>\n</opencv_storage>\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

    public void testWriteYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        extractor.write(filename);

        String truth = "%YAML:1.0\n---\nname: \"Feature2D.BEBLID\"\nscale_factor: 6.7500000000000000e+00\nn_bits: 100\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

}
